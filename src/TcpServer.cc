#include <functional>
#include <string.h>

#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

static EventLoop* checkLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL<<"main Loop is NULL!";
    }
    return loop;
}


TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option)
    : loop_(checkLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop,listenAddr,option = ReusePort))
    , threadPool_(new EventLoopThreadPool(loop,name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
    , started_(0)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}


TcpServer::~TcpServer()
{
    for(auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
}


void TcpServer::setThreadNum(int numThreads)
{
    int numThreads_ = numThreads;
    threadPool_->setThreadNum(numThreads_);
}


void TcpServer::start()
{
    if(started_.fetch_add(1) == 0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}


void TcpServer::newConnection(int sockfd,const InetAddress& peerAddr)
{
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf,sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO<<"TcpServer::newConnection ["<<name_.c_str()<<"]- new connection ["<<connName.c_str()<<"]from "<<peerAddr.toIpPort().c_str();

    sockaddr_in localAddr;
    memset(&localAddr,0,sizeof(localAddr));
    socklen_t len = sizeof(localAddr);
    if(::getsockname(sockfd,(sockaddr*)&localAddr,&len) < 0)
    {
        LOG_ERROR<<"sockets::getLocalAddr";
    }
    InetAddress localAddress(localAddr);
    TcpConnectionPtr conn (new TcpConnection(ioLoop,connName,sockfd,localAddress,peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,conn));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}


void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    conn->getLoop()->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}


void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO<<"TcpServer::removeConnectionInLoop ["<<
             name_.c_str()<<"] - connection %s"<<conn->name().c_str();
    connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}