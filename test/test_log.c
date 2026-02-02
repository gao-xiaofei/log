/**
 * log_test.c - 日志库完整测试示例
 * 编译: gcc -o log_test log_test.c log.c -DLOG_USE_COLOR -lpthread
 */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <pthread.h>

#define sleep_ms(ms) usleep((ms) * 1000)

#define ENABLE_LOG1 1
#define ENABLE_LOG2 0

#if ( ENABLE_LOG1 == 1 )
#define log1_log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log1_log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log1_log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log1_log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log1_log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log1_log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#else
#define log1_log_trace(...) (void)0
#define log1_log_debug(...) (void)0
#define log1_log_info(...)  (void)0
#define log1_log_warn(...)  (void)0
#define log1_log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log1_log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#endif

#if ( ENABLE_LOG2 == 1 )
#define log2_log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log2_log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log2_log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log2_log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log2_log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log2_log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#else
#define log2_log_trace(...) (void)0
#define log2_log_debug(...) (void)0
#define log2_log_info(...)  (void)0
#define log2_log_warn(...)  (void)0
#define log2_log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log2_log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#endif

/**
 * 网络日志回调 - 模拟发送日志到网络服务器
 */
static void network_callback(log_Event *ev) 
{
    static int packet_count = 0;
    
    // 模拟网络包格式
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", ev->time);
    
    // 模拟发送到网络（这里只打印）
    printf("[NETWORK] Packet#%04d | %s | %-5s | %s:%d | ",
           ++packet_count, timestamp, 
           log_level_string(ev->level), ev->file, ev->line);
    
    vprintf(ev->fmt, ev->ap);
    printf("\n");
}


// ==================== 线程锁函数 ====================

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void log_lock_function(bool lock, void *udata) 
{
    (void)udata; 

    if (lock) {
        pthread_mutex_lock(&log_mutex);
    } else {
        pthread_mutex_unlock(&log_mutex);
    }
}

// ==================== 线程函数 ====================

static void *thread_func(void *arg) 
{
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < 5; i++) {
        log_info("[线程%d] 循环 %d", thread_id, i);
        sleep_ms(100);
        
        if (i == 2 && thread_id == 1) {
            log_warn("[线程%d] 模拟警告情况", thread_id);
        }
        
        if (i == 3 && thread_id == 2) {
            log_error("[线程%d] 模拟错误情况: errno=%d", thread_id, errno);
        }
    }
    
    return NULL;
}


// ==================== 边界条件测试 ====================

static void edge_case_test() 
{
    printf("\n=== 边界条件测试 ===\n");
    
    // 测试空消息
    log_info("");
    
    // 测试长消息
    char long_msg[1024];
    memset(long_msg, 'A', sizeof(long_msg) - 1);
    long_msg[sizeof(long_msg) - 1] = '\0';
    log_info("长消息: %s", long_msg);
    
    // 测试特殊字符
    log_info("特殊字符: \\t\\n\\r\\\"\\'\\%");
    
    // 测试格式说明符
    log_info("格式: 字符串=%s, 整数=%d, 浮点数=%.2f, 十六进制=0x%x", 
             "test", 123, 3.14159, 255);
    
    // 测试NULL指针
    char *null_ptr = NULL;
    log_warn("NULL指针测试: %s", null_ptr);
}



// ==================== 多线程测试 ====================

static void multithread_test() 
{
    printf("\n=== 多线程测试 ===\n");
    
    // 设置线程锁
    log_set_lock(log_lock_function, NULL);
    printf("已设置线程锁\n");
    
    pthread_t threads[3];
    int thread_ids[3] = {1, 2, 3};
    
    printf("启动3个线程...\n");
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    // 主线程也记录一些日志
    for (int i = 0; i < 3; i++) {
        log_info("[主线程] 工作 %d", i);
        sleep_ms(150);
    }
    
    // 等待所有线程结束
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("所有线程已完成\n");
}


// ==================== 主函数 ====================

int main() 
{

    printf("========== 日志库示例 ==========\n");

    // 初始配置
    printf("日志库版本: %s\n", LOG_VERSION);
    log_set_level(LOG_TRACE);  // 设置为最详细级别
    log_set_quiet(false);      // 启用控制台输出

    // 基础测试
    printf("\n=== 基础功能测试 ===\n");
    printf("测试所有日志宏...\n");
    log_trace("这是TRACE级别日志");
    log_debug("这是DEBUG级别日志");
    log_info("这是INFO级别日志");
    log_warn("这是WARN级别日志");
    log_error("这是ERROR级别日志");
    log_fatal("这是FATAL级别日志");
    
    log1_log_trace("log1这是TRACE级别日志");
    log1_log_debug("log1这是DEBUG级别日志");
    log1_log_info("log1这是INFO级别日志");
    log1_log_warn("log1这是WARN级别日志");
    log1_log_error("log1这是ERROR级别日志");
    log1_log_fatal("log1这是FATAL级别日志");

    log2_log_trace("log2这是TRACE级别日志");
    log2_log_debug("log2这是DEBUG级别日志");
    log2_log_info("log2这是INFO级别日志");
    log2_log_warn("log2这是WARN级别日志");
    log2_log_error("log2这是ERROR级别日志");
    log2_log_fatal("log2这是FATAL级别日志");
        // 创建日志文件
    FILE *log_file = fopen("test.log", "a");
    if (!log_file) {
        log_error("无法打开日志文件");
    }

    if (log_add_fp(log_file, LOG_INFO) == 0) {
        printf("已添加文件日志: test.log\n");
    } else {
        log_error("无法添加文件日志回调");
    }

    log_add_callback(network_callback, NULL, LOG_INFO);

    // 运行测试
    edge_case_test();
    multithread_test();
    
    // 测试静默模式，即不输出日志到控制台，自定义回调正常输出
    printf("\n=== 静默模式测试 ===\n");
    log_set_quiet(true);
    log_info("这条消息不应该显示");
    log_set_quiet(false);
    log_info("静默模式已关闭，这条消息应该显示");
    

    // 测试不同日志级别字符串
    printf("\n日志级别字符串测试:\n");
    for (int i = LOG_TRACE; i <= LOG_FATAL; i++) {
        printf("级别 %d: %s\n", i, log_level_string(i));
    }
    
    // 模拟程序结束
    log_info("程序正常退出");
    printf("\n所有测试已完成！\n");
    printf("请查看生成的 test.log 文件\n");

    fclose(log_file);
    return 0;
}