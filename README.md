# LT

C + JSON

Light Message Queue Server,  JSON,  IUP, SOCKET, C, WEB 
Based on C language as discussed in the source, JSON as a data structure in a range of applications, 
SOCKET programming, IUP graphics programming, HTTP WEB implementation, message queue server, 
based on Message Queuing, such as design and implementation of application server.

lt_mqs run as windows service, register as follow:

lt_mqs.exe -install


相关电子书 《C + JSON》在豆瓣发布，地址是：
https://read.douban.com/ebook/24885643/

感兴趣的朋友可以移驾去看看。

导言：

C 语言相对其他语言来说编程难度较大，这主要体现在 C 语言偏重底层，语法不够丰富，指针等概念难于理解等。本文总结了作者多年来的 C 语言编程经验，提出了 C 语言结合 JSON 这种数据类型，能够有效的降低其编程难度。 这本书以 C 和 JSON 为基础，讨论了 SOCKET 编程， IUP 图形编程，HTTP WEB 实现， 消息队列以及以消息队列为核心的应用服务器的设计和实现等内容。本书结合存放在 Github 上的源码程序，能够使得读者知行合一，较快的理解和掌握以 C 语言和 JSON 为基础的一系列应用。

本书由浅入深，首先介绍了 C 语言中的三个重点概念，分别是字符串，指针，函数，说明了 JSON 数据类型在这三个方面的起到的便利作用。

以 C + JSON 基础，首先设计和实现了一个 SOCKET 函数库，该函数库基于 SELECT 模型，屏蔽了 SOCKET 的底层通信，程序员只需要关心通信报文的设计和实现就可以完成服务端和客户端的编程工作。

其次，介绍了 IUP 图形编程的方法，IUP 是一个纯粹的 C 语言图形编程框架，并具有跨平台功能。结合 SOCKET 函数库，实现了一个简单的消息聊天的系统。

随后，讨论了 HTTP 的一些概念，并结合 JSON，实现了一个 HTTP WEB 服务器编程框架，包括服务端和客户端，可以方便的嵌入到其他程序中。

最后，重点设计和实现了消息队列，以及以消息队列为基础的应用服务器。消息服务器采用 SQLITE3 数据库保存消息，分为文件型和内存型两种，满足可靠消息和实时消息的要求。应用服务器类似 TUXEDO 中间件，可以把业务逻辑封装在该应用服务器管理的服务模块中。

目录信息：

    前言
    第一章 C语言
    1.1 概述
    1.2 字符串
    1.2.1 字符串类型
    1.2.2 字符串函数
    1.2.3 字符集的转换
    1.3 指针
    1.3.1 指针类型
    1.3.2 指针链表
    1.3.3 野指针与多线程
    1.4 函数的参数
    1.5 static关键字
    第二章 JSON
    2.1 概述
    2.2 JSON-C
    2.3 序列化和解析
    2.3.1. JSON-C函数
    2.3.2 应用场景
    2.4 代替数据结构
    2.5 函数的参数
    第三章 LT_SOCKET
    3.1 概述
    3.2 socket报文
    3.2.1 报文定义
    3.2.2 报文序列化和解析
    3.3 线程与同步
    3.4 LT_SERVERSOCKET服务器
    3.4.1 SELECT模型
    3.4.2 socket server
    3.4.3 服务端编写方法
    3.5 LT_CLIENTSOCKET客户端
    3.5.1 SOCKET客户端
    3.5.2 SOCKET客户端的封装实现
    3.5.3 客户端编写方法
    第四章 IUP图形编程
    4.1 概述
    4.2 IUP介绍
    4.3 IUP实例
    4.3.1 程序功能
    4.3.2 报文通信协议
    4.4 服务端对话框
    4.4.1 托盘程序
    4.4.2 图标资源
    4.4.3 菜单，工具条，状态栏
    4.4.4 表格控件
    4.4.5 服务端报文处理
    4.4.6 main函数
    4.5 客户端程序
    4.5.1 报文解析序列化
    4.5.2 界面组织
    4.5.3 客户端链接
    第五章 HTTP服务器
    5.1 概述
    5.2 服务器设计实现
    5.2.1 HTTP报文格式
    5.2.2 HTTP报文和JSON对象的转换
    5.2.3 URL编码
    5.3 客户端设计实现
    5.3.1 报文处理
    5.3.2 GET方法实现
    第六章 消息中间件
    6.1 概述
    6.2 队列和消息
    6.3 SQLITE3
    6.4 消息中间件
    6.4.1 概述
    6.4.2 通信协议
    6.4.3 服务端LT_SERVER
    6.4.4 请求处理
    6.5 客户端API接口
    6.6 管理程序
    第七章 应用服务器
    7.1 概述
    7.2 命名服务器设计实现
    7.3 服务端API设计
    7.4 客户端API设计
    7.5 应用场景
    7.5.1 用户登录模块
    7.5.2 用户表格展现
    7.5.3 IupMatrix表格控件
    7.5.4 IUP工具条的实现
    7.5.5 UserClient界面布局
    7.5.6 lt_record记录集及SQL提取
    7.5.7 UserClient的增删改
