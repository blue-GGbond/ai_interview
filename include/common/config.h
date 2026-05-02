/*
单例管理所有配置参数
从json文件加载配置
生成startsession请求json
*/

#pragma once

#include <string>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace interview
{
	namespace common
	{ 
		// 时间常量
		namespace timing
		{
			// Websocket 连接稳定等待时间
			constexpr auto CONNECTION_STABILIZE_DELAY=std::chrono::milliseconds(100);

			// 面试结束后延迟关闭会话的时间
			constexpr auto INTERVIEW_END_DELAY=std::chrono::seconds(5);

			// 麦克风音频发送间隔
			constexpr auto AUDIO_SEND_INTERVAL=std::chrono::milliseconds(10);

			// 错误重试延迟时间
			constexpr auto ERROR_RETRY_DELAY=std::chrono::milliseconds(100);

			// 面试开始前的准备延迟时间
			constexpr auto INTERVIEW_START_DELAY = std::chrono::seconds(3);

			// 主循环轮询间隔
			constexpr auto MAIN_LOOP_INTERVAL = std::chrono::milliseconds(100);

			// 音频队列等待超时时间
			constexpr auto AUDIO_QUEUE_WAIT = std::chrono::seconds(1);
		}

		/**
		* @brief 音频配置结构
		* 定义音频流的各项参数，用于配置PortAudio音频设备和websocket传输
		* 输入的麦克风和输出的扬声器使用不同的配置
		* 
		* 说明:
		* - chunk: 每次读取/写入的采样帧数，影响延迟和缓冲区大小
		* - channels: 声道数（1=单声道，2=立体声）
		* - sample_rate: 采样率（Hz），常见值16000/24000/44100/48000
		* - bit_size: 每个采样的位数（8/16/24/32），影响音质和数据量
		* - format: 音频格式标识，通常为"pcm"（脉冲编码调制）
		*/
		struct AudioConfig
		{
			int chunk=0; // 缓冲区大小(采样帧数)，由配置文件提供
            int channels=0; // 声道数
            int sample_rate=0; // 采样率
            int bit_size=0; // 每个采样的位数
            std::string format=""; // 音频格式标识
		};

		/**
		* @brief websocket配置结构
		* 包括服务器地址和必需的HTTP头（认证信息）。
		* 
		* 认证说明：
		* - X-Api-App-ID: 应用ID，标识调用方应用
		* - X-Api-Access-Key: 访问密钥，用于身份验证
		* - X-Api-Resource-Id: 资源ID，指定使用的服务类型
		* - X-Api-App-Key: 应用密钥，用于签名验证
		* - X-Api-Connect-Id: 连接ID，UUID格式，唯一标识本次连接
		*/
		struct WebSocketConfig
		{
			std::string base_url; // websocket服务器地址（wss加密连接）
			std::map<std::string, std::string> headers; // HTTP请求头,包含认证信息

			// 构造函数，初始化认证头信息
			WebSocketConfig();
		};

		/**
		* @brief 对话配置
		* 
		* 配置AI对话机器人的行为、角色和说话风格。
		* 
		* 核心理念：
		* - system_role: 定义AI的核心职责和禁止行为（最重要的配置）
		* - speaking_style: 定义AI的说话风格和表达方式
		* - bot_name: AI的名称，用于自我介绍
		*/
		struct DialogConfig
		{
			std::string bot_name; // 机器人名称
            std::string system_role; // 系统角色描述，描述AI的核心职责和禁止行为
            std::string speaking_style; // 说话风格描述，描述AI的说话风格和表达方式
			std::string city; // 城市名称,可以影响ai的口音
			bool strict_audit=false; // 是否开启严格审核
			std::string audit_response; // 审核触发时的回复语
			int recv_timeout=0; // 接收消息超时时间
			std::string input_mod; // 输入模式: "text" 或 "audio"，文本和语音输入方式

			// 构造函数，初始化对话配置
            DialogConfig();
		};

		/**
		* @brief TTS(文本转语言)配置
		* 
		* 配置AI语音合成的参数，控制返回的音频流格式和说话人声音。
		* 
		* 说话人选择:
		* - "zh_male_yunzhou_jupiter_bigtts": 中文男声，适合面试官角色
		* 
		* 音频格式选择:
		* - 24000Hz采样率：高清语音质量，适合对话场景
		* - 单声道：减少带宽，语音应用通常不需要立体声
		* - PCM格式：未压缩，延迟低，适合实时对话
		*/
		struct TTSConfig
		{
			std::string speaker; // 说话人id
			int channel = 0; // 声道数
			std::string format; // 音频格式: "pcm"
            int sample_rate = 0; // 采样率
		};

		/**
		*  @brief ASR（语音识别转文本）配置结构
		* 配置语音识别引擎的VAD（语音活动检测）参数，影响何时检测到说话开始/结束。
		*
		* VAD工作原理：
		* 1. 检测到连续语音超过speech_trigger_duration -> 触发"开始说话"
		* 2. 检测到连续静音超过silence_duration -> 触发"停止说话"
		* 3. 在停止说话前，使用end_smooth_window平滑处理，避免误判
		*
		* 参数调优建议：
		* - 增大silence_duration：减少误判（允许停顿思考），但响应变慢
		* - 减小silence_duration：快速响应，但可能截断长句子
		* - 增大speech_trigger_duration：减少噪声误触发，但启动变慢
		* - end_smooth_window：平滑窗口越大，结束检测越稳定
		*
		* 面试场景优化：
		* - 1000ms静音：允许候选人思考停顿，避免打断回答
		* - 400ms触发：快速检测开始说话，提升交互体验
		* - 1500ms平滑：确保完整句子识别，不会因短暂停顿截断
		*/
		struct ASRConfig
		{
			int end_smooth_window_ms = 0; // 平滑处理语言结束检测
			int vad_silence_duration = 0; // VAD静音持续时间
			int vad_speech_trigger_duration = 0; // VAD语言触发持续时间
		};

		/**
		* @brief LLM API配置结构
		* 
		* 配置大语言模型API的连接参数和生成参数。
		* 项目使用LLM进行三个核心功能：
		* 1. 根据简历生成面试问题
		* 2. 实时评估候选人回答并判断是否追问
		* 3. 生成面试总结报告
		*/
		struct LLMConfig
		{ 
            std::string api_url; // LLM API地址
            std::string api_key; // LLM API密钥
			std::string model; // LLM模型名称
			float temperature = 0.0f; // 模型温度参数（控制输出随机性（0.0=确定性，2.0=高创造性））
            int max_tokens = 0; // 模型最大生成token数
			int timeout_seconds = 0; // HTTP请求超时时间

			// 构造函数，初始化LLM配置
            LLMConfig();
		};

		/**
		* @brief 全局配置管理类（单例模式）
		*
		* 统一管理系统所有配置参数，确保全局唯一的配置实例。
		* 使用单例模式避免配置重复初始化和不一致问题。
		*
		* 设计模式：
		* - 单例模式（Singleton）：确保全局唯一实例
		* - 延迟初始化（Lazy Initialization）：首次访问时才创建实例
		* - 禁用拷贝：防止意外复制导致多实例
		*
		* 配置组织：
		* 1. WebSocket配置：连接认证和服务器地址
		* 2. 音频配置：输入（麦克风）和输出（扬声器）分开配置
		* 3. 对话配置：AI角色、风格、审核等
		* 4. TTS配置：语音合成参数
		* 5. ASR配置：语音识别VAD参数
		* 6. LLM配置：大模型API参数
		*
		* 使用方式：
		* @code
		* auto& cfg = Config::Instance();
		* cfg.llm_config.api_key = "your-key";
		* @endcode
		*/
		class Config
		{
		public:
			// 获取单例实例
			static Config& Instance();

			// websocket配置
			WebSocketConfig ws_config;

			// 音频配置(分别输入和输出)
			AudioConfig input_audio_config; // 输入音频配置,16000HZ单声道
            AudioConfig output_audio_config; // 输出音频配置,24000Hz单声道（匹配TTS）

			// 对话配置
            DialogConfig dialog_config;

			// TTS配置
            TTSConfig tts_config;

            // ASR配置
            ASRConfig asr_config;

            // LLM配置
            LLMConfig llm_config;

			/**
			* @brief 从JSON配置文件加载所有设置
			*
			* @param path 配置文件路径
			* @throws std::runtime_error 文件不存在或解析失败时抛出
			*/
			void LoadFromFile(const std::string& path);

			/**
			* @brief 生成StartSession请求的JSON
			*
			* 将所有对话相关配置（ASR、TTS、Dialog）打包成StartSession请求的payload。
			* 这个JSON会在建立会话时发送给服务器，配置AI的行为和音频参数。
			*
			* JSON结构：
			* {
			*   "asr": { "extra": { VAD参数 } },
			*   "tts": { "speaker": ..., "audio_config": { 音频格式 } },
			*   "dialog": { "bot_name": ..., "system_role": ..., "speaking_style": ..., "extra": { 其他参数 } }
			* }
			*
			* @return StartSession请求的完整JSON payload
			*/
			nlohmann::json GenerateStartSessionRequest() const;

			/**
			* @brief 验证配置参数的有效性
			*
			* 检查所有配置参数是否在有效范围内，包括：
			* - 音频参数（采样率、声道数、缓冲区大小）
			* - WebSocket URL格式
			* - LLM配置（温度、token数、超时）
			* - VAD配置（静音/语音触发时长）
			*
			* @throws std::runtime_error 如果任何配置参数无效
			*/
			void ValidateConfiguration() const;

		private:
			// 私有构造函数，确保单例模式
            Config();

			// 私有拷贝构造函数，防止意外复制
            Config(const Config&) = delete;

			// 禁用赋值操作符，防止意外赋值
			Config& operator=(const Config&) = delete;
		};
	}
}