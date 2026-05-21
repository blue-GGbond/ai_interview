#include "interview/dialog_session.h"
#include "services/audio_manager.h"
#include "services/realtime_client.h"
#include "interview/interview_manager.h"
#include "common/config.h"
#include "common/logger.h"
#include "common/interview_state.h"
#include "common/protocol.h"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <string>

namespace interview
{
	namespace session
	{
		class DialogSession::DialogSessionImpl
		{
		public:
			std::unique_ptr<services::AudioDeviceManager> audio_manager;
			std::unique_ptr<services::RealtimeClient> realtime_client;
			std::shared_ptr<InterviewSession> interview_session;

			// 流程状态
			bool is_intro_done;
			std::atomic<bool> is_running;
			std::atomic<bool> is_playing_audio;
			std::atomic<int> tts_cnt;

			// 待处理的答案
			std::string pending_answer;
			std::mutex pending_answer_mutex;

			std::string final_summary;
			std::mutex final_summary_mutex;

			// 对话内容回调
			DialogSession::DialogContentCallback dialog_content_callback;

			std::thread microphone_thread; //麦克风线程
			std::thread playback_thread;

			std::queue<std::vector<float>> audio_queue; //音频队列
			std::mutex audio_queue_mutex;
			std::condition_variable audio_queue_cv;

			DialogSessionImpl(const std::string& name)
				:is_intro_done(false)
				, is_running(false)
				, is_playing_audio(false)
				, tts_cnt(0)
			{
				interview_session = std::make_shared<InterviewSession>(name);

				// 初始化全局状态机
				common::InterviewStateMachine::Instance().Reset();
			}

			~DialogSessionImpl()
			{
				Stop();
			}

			void TransitionToState(common::InterviewState new_state)
			{
				common::InterviewStateMachine::Instance().SetState(new_state);
				LOG_INFO("状态：%s", common::InterviewStateMachine::GetStateName(new_state));
			}

			void Start()
			{
				if (is_running)
				{
					LOG_WARNING("Session already running");
					return;
				}

				auto& cfg = common::Config::Instance();

				// 创建音频管理器，并且配置
				services::AudioConfig input_cfg;
				input_cfg.sample_rate = cfg.input_audio_config.sample_rate;
				input_cfg.channels = cfg.input_audio_config.channels;
				input_cfg.chunk = cfg.input_audio_config.chunk;

				services::AudioConfig output_cfg;
				output_cfg.sample_rate = cfg.output_audio_config.sample_rate;
				output_cfg.channels = cfg.output_audio_config.channels;
				output_cfg.chunk = cfg.output_audio_config.chunk;

				audio_manager = std::make_unique<services::AudioDeviceManager>(input_cfg, output_cfg);

				// 创建websocket客户端
				auto& ws_cfg = cfg.ws_config;
				realtime_client = std::make_unique<services::RealtimeClient>(ws_cfg.base_url, ws_cfg.headers);

				// 设置响应回调
				realtime_client->SetResponseCallback([this](const common::ParsedResponse& resp)
					{
						HandleServerResponse(resp);
					});

				// 连接到服务器
				LOG_INFO("Connecting to server...");
				TransitionToState(common::InterviewState::kConnecting);
				realtime_client->Connect();

				// 等待一段时间让其连接
				std::this_thread::sleep_for(common::timing::CONNECTION_STABILIZE_DELAY);

				// 打开音频流
				LOG_INFO("Opening audio streams...");
				audio_manager->OpenInputStream();
				audio_manager->OpenOutputStream();

				is_running = true;

				// 启动播放线程
				playback_thread = std::thread([this]()
					{
						PlaybackThreadFunc();
					});

				// 启动麦克风线程
				microphone_thread = std::thread([this]()
					{
						MicrophoneThreadFunc();
					});

				is_playing_audio = true;

				// 发送开场白
				if (dialog_content_callback)
				{
					dialog_content_callback("interviewer", "你好，欢迎参加今天的面试，先做一个简单的自我介绍。", 0);
				}
				std::string intro = interview_session->GetIntroPrompt();
				SendInterviewerPrompt(intro);

				LOG_INFO("Dialog session started");
			}

			void Stop()
			{
				if (!is_running)
				{
					return;
				}

				LOG_INFO("Stopping dialog session");
				is_running = false;

				audio_queue_cv.notify_all(); //唤醒所有线程

				// 等待线程结束
				if (microphone_thread.joinable())
				{
					microphone_thread.join();
				}

				if (playback_thread.joinable())
				{
					microphone_thread.join();
				}

				// 关闭websocket
				if (realtime_client)
				{
					realtime_client->Close();
				}

				// 清理音频
				if (audio_manager)
				{
					audio_manager->Cleanup();
				}

				// 保存面试报告
				if (interview_session)
				{
					try
					{
						std::string report_file = interview_session->SaveReport();
						LOG_INFO("Interview report saved: {}", report_file);
					}
					catch (const std::exception& e)
					{
						LOG_ERROR("Failed to save report: {}", e.what());
					}
				}

				if (interview_session)
				{
					std::string summary_copy;
					{
						std::lock_guard<std::mutex> summay_lock(final_summary_mutex);
						summary_copy = final_summary;
					}

					if (summary_copy.empty() && interview_session)
					{
						try
						{
							summary_copy = interview_session->GenerateSummary();
							if (!summary_copy.empty())
							{
								std::lock_guard<std::mutex> summary_lock(final_summary_mutex);
								final_summary = summary_copy;
							}
						}
						catch (const std::exception& e)
						{
							LOG_ERROR("Failed to generate summary during shutdown: {}", e.what());
						}
					}

					if (!summary_copy.empty())
					{
						LogMultilineBlock("[面试总结回顾]", summary_copy);
					}
				}

				LOG_INFO("Dialog session stopped");
			}
		};
	}
}