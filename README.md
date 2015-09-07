cctools: 一些常用的C++轮子
===

conf.h
---
***用于配置文件读取***

logger.h
---
***用于写日志，支持5个日志级别***

    DEBUG
    INFO
    WARN
    ERROR
    CRIT
    
threadSafeMap.h
---
***线程安全map***

pool.h
---
***内存池***: 自定义对象需要继承cctools::Base，使用NewClassFromPool这个宏去实现new操作，就可以把new出来的指针加入到内存池中，待内存池被释放的时候统一释放

***以上几个功能的示例代码可以参考：***

    test/main_test.cc


net.h
---
***一个简单的多路复用IO框架，支持epoll和kqueue***

示例代码：

    test/net_test.cc
    test/net_ngx.conf // 一个简单的nginx server配置，在http块中include即可
    
    // 编译cctools库和示例文件
    make
    make test
    
    // 运行net示例
    bin/net_test
    
    // 另开一个窗口，配置好nginx，在nginx.conf中引入net_ngx.conf（ngx需要支持ngx-lua模块）
    http {
        # some other configs
        include path/to/cctools/test/net_ngx.conf;
    }
    
    /path/to/nginx -c /path/to/nginx.conf
    
    curl "127.0.0.1:16667/tcp?key1=val1&key2=val2"
    
