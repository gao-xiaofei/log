/**
 * log_test.c - 日志库完整测试示例
 * 编译: gcc -o log_test log_test.c log.c -DLOG_USE_COLOR -lpthread
 */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <sys/time.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

// ==================== 自定义回调函数示例 ====================

/**
 * 网络日志回调 - 模拟发送日志到网络服务器
 */
static void network_callback(log_Event *ev) {
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

/**
 * 数据库日志回调 - 模拟存储日志到数据库
 */
static void database_callback(log_Event *ev) {
    // 模拟SQL语句格式
    printf("[DATABASE] INSERT INTO logs(level,file,line,message) VALUES(");
    printf("'%s','%s',%d,'", 
           log_level_string(ev->level), ev->file, ev->line);
    
    // 处理消息中的单引号
    char buf[256];
    vsnprintf(buf, sizeof(buf), ev->fmt, ev->ap);
    for (char *p = buf; *p; p++) {
        if (*p == '\'') printf("''");
        else printf("%c", *p);
    }
    printf("');\n");
}

/**
 * JSON格式日志回调
 */
static void json_callback(log_Event *ev) {
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", ev->time);
    
    char message[256];
    vsnprintf(message, sizeof(message), ev->fmt, ev->ap);
    
    // 转义JSON特殊字符
    char escaped[512];
    char *dest = escaped;
    for (const char *src = message; *src && dest < escaped + sizeof(escaped) - 2; src++) {
        switch (*src) {
            case '"': *dest++ = '\\'; *dest++ = '"'; break;
            case '\\': *dest++ = '\\'; *dest++ = '\\'; break;
            case '\b': *dest++ = '\\'; *dest++ = 'b'; break;
            case '\f': *dest++ = '\\'; *dest++ = 'f'; break;
            case '\n': *dest++ = '\\'; *dest++ = 'n'; break;
            case '\r': *dest++ = '\\'; *dest++ = 'r'; break;
            case '\t': *dest++ = '\\'; *dest++ = 't'; break;
            default: *dest++ = *src;
        }
    }
    *dest = '\0';
    
    printf("[JSON] {\"timestamp\":\"%s\",\"level\":\"%s\","
           "\"file\":\"%s\",\"line\":%d,\"message\":\"%s\"}\n",
           timestamp, log_level_string(ev->level), 
           ev->file, ev->line, escaped);
}

// ==================== 线程锁函数 ====================

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void log_lock_function(bool lock, void *udata) {
    (void)udata; // 未使用参数
    if (lock) {
        pthread_mutex_lock(&log_mutex);
    } else {
        pthread_mutex_unlock(&log_mutex);
    }
}

// ==================== 线程函数 ====================

static void *thread_func(void *arg) {
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

// ==================== 性能测试 ====================

static void performance_test() {
    printf("\n=== 性能测试 ===\n");
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    int iterations = 10000;
    for (int i = 0; i < iterations; i++) {
        log_debug("性能测试消息 %d", i);
    }
    
    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds * 1e-6;
    
    printf("日志次数: %d\n", iterations);
    printf("总时间: %.3f 秒\n", elapsed);
    printf("平均每条: %.3f 微秒\n", elapsed * 1e6 / iterations);
    printf("每秒: %.0f 条\n", iterations / elapsed);
}

// ==================== 边界条件测试 ====================

static void edge_case_test() {
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

// ==================== 日志级别测试 ====================

static void log_level_test() {
    printf("\n=== 日志级别测试 ===\n");
    
    // 测试所有日志级别
    log_set_level(LOG_TRACE);
    printf("\n当前日志级别: TRACE\n");
    log_trace("跟踪消息");
    log_debug("调试消息");
    log_info("信息消息");
    log_warn("警告消息");
    log_error("错误消息");
    log_fatal("致命消息");
    
    log_set_level(LOG_INFO);
    printf("\n当前日志级别: INFO\n");
    log_trace("跟踪消息（不应显示）");
    log_debug("调试消息（不应显示）");
    log_info("信息消息");
    log_warn("警告消息");
    log_error("错误消息");
    log_fatal("致命消息");
    
    log_set_level(LOG_ERROR);
    printf("\n当前日志级别: ERROR\n");
    log_trace("跟踪消息（不应显示）");
    log_debug("调试消息（不应显示）");
    log_info("信息消息（不应显示）");
    log_warn("警告消息（不应显示）");
    log_error("错误消息");
    log_fatal("致命消息");
}

// ==================== 文件日志测试 ====================

static void file_logging_test() {
    printf("\n=== 文件日志测试 ===\n");
    
    // 创建日志文件
    FILE *log_file = fopen("test.log", "a");
    if (!log_file) {
        log_error("无法打开日志文件");
        return;
    }
    
    // 添加文件日志
    if (log_add_fp(log_file, LOG_INFO) == 0) {
        printf("已添加文件日志: test.log\n");
        
        // 写入一些测试日志
        log_info("文件日志测试 - 开始");
        log_warn("这是一个警告消息");
        log_error("这是一个错误消息");
        
        // 模拟程序运行
        for (int i = 0; i < 3; i++) {
            log_info("程序运行中... 迭代 %d", i + 1);
            sleep_ms(50);
        }
        
        log_info("文件日志测试 - 结束");
        
        printf("\n文件日志已写入，请查看 test.log\n");
    } else {
        log_error("无法添加文件日志回调");
    }
    
    fclose(log_file);
}

// ==================== 自定义回调测试 ====================

static void callback_test() {
    printf("\n=== 自定义回调测试 ===\n");
    
    // 添加各种自定义回调
    log_add_callback(network_callback, NULL, LOG_INFO);
    log_add_callback(database_callback, NULL, LOG_WARN);
    log_add_callback(json_callback, NULL, LOG_ERROR);
    
    printf("已添加3个自定义回调:\n");
    printf("1. 网络日志回调 (INFO+)\n");
    printf("2. 数据库日志回调 (WARN+)\n");
    printf("3. JSON格式回调 (ERROR+)\n\n");
    
    // 测试不同级别的消息
    log_info("用户 'admin' 登录系统");
    log_warn("磁盘空间不足: 剩余 %.1f GB", 1.5);
    log_error("数据库连接失败: %s", "Connection refused");
    log_fatal("系统崩溃: 内存不足");
    
    printf("\n注意：不同级别的消息被不同回调处理\n");
}

// ==================== 多线程测试 ====================

static void multithread_test() {
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

// ==================== 错误处理测试 ====================

static void error_handling_test() {
    printf("\n=== 错误处理测试 ===\n");
    
    // 测试回调添加失败（超过最大限制）
    printf("尝试添加过多回调...\n");
    
    // 先添加一些回调
    for (int i = 0; i < 32; i++) {
        int result = log_add_callback(stdout_callback, stderr, LOG_INFO);
        if (result != 0) {
            printf("第 %d 个回调添加失败 (预期行为)\n", i + 1);
            break;
        }
    }
    
    // 测试无效参数
    printf("\n测试无效参数...\n");
    log_log(-1, __FILE__, __LINE__, "无效日志级别测试");
    log_log(100, __FILE__, __LINE__, "超范围日志级别测试");
    
    // 恢复
    log_set_quiet(false);
}

// ==================== 实际应用场景模拟 ====================

static void real_world_scenario() {
    printf("\n=== 实际应用场景模拟 ===\n");
    
    // 模拟Web服务器日志
    printf("\n[场景1] Web服务器日志\n");
    log_info("服务器启动在端口 8080");
    log_info("客户端 192.168.1.100 连接到 /api/users");
    log_info("GET /api/users?page=1 - 200 OK - 时间: 15ms");
    log_warn("客户端 192.168.1.101 频繁请求 /api/login");
    log_error("数据库查询失败: %s", "表 'users' 不存在");
    
    // 模拟游戏服务器日志
    printf("\n[场景2] 游戏服务器日志\n");
    log_info("玩家 'Player1' 加入游戏");
    log_info("玩家 'Player2' 加入游戏");
    log_debug("玩家位置更新: Player1 (x=100, y=50)");
    log_warn("玩家 'Player1' 使用异常速度移动");
    log_info("玩家 'Player1' 离开游戏");
    log_error("网络同步失败: Player2 数据包丢失");
    
    // 模拟IoT设备日志
    printf("\n[场景3] IoT设备日志\n");
    log_info("设备启动 - 固件版本: v1.2.3");
    log_debug("传感器读数: 温度=25.3℃, 湿度=60%%");
    log_info("连接到WiFi: SSID='HomeNetwork'");
    log_warn("电池电量低: 15%%");
    log_error("MQTT连接断开: 重新连接中...");
    log_info("数据上传成功: 消息ID=0x1234ABCD");
}

// ==================== 主函数 ====================

int main() {
    printf("========== 日志库完整测试 ==========\n");
    
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
    
    // 运行各个测试
    log_level_test();
    edge_case_test();
    callback_test();
    file_logging_test();
    multithread_test();
    error_handling_test();
    real_world_scenario();
    performance_test();
    
    // 测试静默模式
    printf("\n=== 静默模式测试 ===\n");
    log_set_quiet(true);
    log_info("这条消息不应该显示");
    log_set_quiet(false);
    log_info("静默模式已关闭，这条消息应该显示");
    
    // 最终清理测试
    printf("\n=== 测试完成 ===\n");
    
    // 测试不同日志级别字符串
    printf("\n日志级别字符串测试:\n");
    for (int i = LOG_TRACE; i <= LOG_FATAL; i++) {
        printf("级别 %d: %s\n", i, log_level_string(i));
    }
    
    // 模拟程序结束
    log_info("程序正常退出");
    printf("\n所有测试已完成！\n");
    printf("请查看生成的 test.log 文件\n");
    
    return 0;
}