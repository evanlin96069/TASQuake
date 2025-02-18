#include "libtasquake/boost_ipc.hpp"
#include "libtasquake/utils.hpp"
#include <array>
#include <chrono>
#include <iostream>
#include <thread>

using namespace ipc;

ipc::session::session(boost::asio::ip::tcp::socket socket, size_t connection_id, ipc::server* owner)
: socket_(std::move(socket)), connection_id(connection_id), owner_(owner)
 { }
        
void ipc::session::start() {
    do_read();
}

static void write_to_socket(boost::asio::ip::tcp::socket& socket, void* data, size_t size) {
    uint8_t* ptr = (uint8_t*)data;
    int32_t total_bytes = size;
    size_t written = 0;
    while(total_bytes > 0) {
        size_t bytes = socket.write_some(boost::asio::buffer(ptr + written, total_bytes));
        total_bytes -= bytes;
        written += bytes;
    }
}

void ipc::session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(sockbuf, sizeof(sockbuf)),
    [this, self](boost::system::error_code ec, std::size_t length)
    {
    if (!ec)
    {
        uint8_t* buffer = (uint8_t*)sockbuf;
        while(length > 0) {
            if(this->m_currentMessage.length == 0) {
                if(length < 4) {
                    TASQuake::Log("Message was too short, did not contain length!\n");
                    goto end;
                } 

                uint32_t size_bytes;
                memcpy(&size_bytes, buffer, sizeof(uint32_t));
                const uint32_t MAX_BYTES = 1 << 25;
                if(size_bytes > MAX_BYTES) {
                    TASQuake::Log("Message exceeded the maximum size: %u > %u\n", size_bytes, MAX_BYTES);
                    this->m_currentMessage.address = nullptr;
                } else {
                    this->m_currentMessage.address = malloc(size_bytes);
                }
                this->m_currentMessage.connection_id = this->connection_id;
                this->m_currentMessage.length = size_bytes;
                buffer += sizeof(uint32_t);
                length -= 4;
            } else {
                size_t bytesToRead = std::min(this->m_currentMessage.length - this->m_uBytesWritten, length);
                if(this->m_currentMessage.address)
                        memcpy((uint8_t*)this->m_currentMessage.address + this->m_uBytesWritten, buffer, bytesToRead);
                this->m_uBytesWritten += bytesToRead;
                length -= bytesToRead;
                buffer += bytesToRead;

                if(this->m_currentMessage.length == this->m_uBytesWritten) {
                    if(this->m_currentMessage.address) {
                        this->owner_->message_from_connection(this->m_currentMessage);
                    }
                    this->m_currentMessage.address = nullptr;
                    this->m_currentMessage.connection_id = 0;
                    this->m_currentMessage.length = 0;
                    this->m_uBytesWritten = 0;
                } else if (this->m_currentMessage.length < this->m_uBytesWritten) {
                    TASQuake::Log("A bad happened\n");
                    abort();
                }
            }
        }
        end:
        this->do_read();
    }
    else {
        // Connection terminated, delete the session
        self->owner_->delete_session(self);
    }
    });
}

void ipc::server::run_service() {
    service_->run();
}

void ipc::server::start(short port) {
    service_ = new boost::asio::io_service();
    acceptor_ = new boost::asio::ip::tcp::acceptor(*service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    socket_ = new boost::asio::ip::tcp::socket(*service_);

    do_accept();
    m_bConnected = true;
    m_tThread = std::thread(std::bind(&ipc::server::run_service, this));
}

void ipc::server::stop() {
    m_bConnected = false;
    service_->stop();
    m_tThread.join();
    delete service_;
    delete acceptor_;
    delete socket_;
    service_ = nullptr;
    acceptor_ = nullptr;
    socket_ = nullptr;
    sessions_.clear();
}

void ipc::server::delete_session(std::shared_ptr<session> ptr) {
    std::lock_guard<std::mutex> guard(message_mutex);
    sessions_.erase(std::remove(sessions_.begin(), sessions_.end(), ptr));
}

std::shared_ptr<session> ipc::server::get_session(size_t connection_id) {
    std::lock_guard<std::mutex> guard(message_mutex);
    for(auto ptr : sessions_) {
      if(ptr->connection_id == connection_id) {
        return ptr;
      }
    }

    return nullptr;
}

void ipc::server::get_messages(std::vector<Message>& messages) {
    std::lock_guard<std::mutex> guard(message_mutex);
    messages = this->messages;
    this->messages.clear();
}

void ipc::server::send_message(size_t connection_id, void* data, uint32_t size) {
    auto sess = get_session(connection_id);
    write_to_socket(sess->socket_, &size, sizeof(uint32_t));
    write_to_socket(sess->socket_, data, size);
}

void ipc::server::message_from_connection(Message msg) {
    std::lock_guard<std::mutex> guard(message_mutex);
    messages.push_back(msg);
}


void ipc::server::get_sessions(std::vector<size_t>& session_ids) {
    std::lock_guard<std::mutex> guard(message_mutex);
    session_ids.clear();
    for(auto& session : sessions_) {
        session_ids.push_back(session->connection_id);
    }
}

void ipc::server::do_accept() {
    acceptor_->async_accept(*socket_,
    [this](boost::system::error_code ec)
    {
        socket_->set_option(boost::asio::ip::tcp::no_delay(true));
        if (!ec)
        {
            size_t connection_id = this->new_session_id;
            ++this->new_session_id;
            {
                std::lock_guard<std::mutex> guard(message_mutex);
                auto ptr = std::make_shared<session>(std::move(*socket_), connection_id, this);
                ptr->start();
                sessions_.push_back(ptr);
            }
        }

        do_accept();
    });
}

ipc::client::client() = default;

void ipc::client::get_messages(std::vector<Message>& messages, size_t timeoutMsec) {
    auto start = std::chrono::steady_clock::now();

    while(true) {
        {
            std::lock_guard<std::mutex> guard(this->message_mutex);
            if(!this->messages_.empty()) {
                for(auto& msg :this->messages_) {
                    messages.push_back(msg);
                }
                this->messages_.clear();
                return;
            }
        }

        auto now = std::chrono::steady_clock::now();

        if((now - start).count() / 1e6 > timeoutMsec) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void ipc::client::do_read() {
    socket_->async_read_some(boost::asio::buffer(sockbuf, sizeof(sockbuf)),
    [this](boost::system::error_code ec, std::size_t length)
    {
        if (!ec)
        {
            uint8_t* buffer = (uint8_t*)sockbuf;
            while(length > 0) {
                if(this->m_currentMessage.length == 0) {
                    if(length < 4) {
                        TASQuake::Log("Message was too short, did not contain length!\n");
                        goto end;
                    } 

                    uint32_t size_bytes;
                    memcpy(&size_bytes, buffer, sizeof(uint32_t));
                    const uint32_t MAX_BYTES = 1 << 25;
                    if(size_bytes > MAX_BYTES) {
                        TASQuake::Log("Message exceeded the maximum size: %u > %u\n", size_bytes, MAX_BYTES);
                        this->m_currentMessage.address = nullptr;
                    } else {
                        this->m_currentMessage.address = malloc(size_bytes);
                    }
                    this->m_currentMessage.connection_id = 0;
                    this->m_currentMessage.length = size_bytes;
                    buffer += sizeof(uint32_t);
                    length -= 4;
                } else {
                    size_t bytesToRead = std::min(this->m_currentMessage.length - this->m_uBytesWritten, length);
                    if(this->m_currentMessage.address)
                        memcpy((uint8_t*)this->m_currentMessage.address + this->m_uBytesWritten, buffer, bytesToRead);
                    this->m_uBytesWritten += bytesToRead;
                    length -= bytesToRead;
                    buffer += bytesToRead;

                    if(this->m_currentMessage.length == this->m_uBytesWritten) {
                        if(this->m_currentMessage.address) {
                            std::lock_guard<std::mutex> guard(this->message_mutex);
                            this->messages_.push_back(this->m_currentMessage);
                        }
                        this->m_currentMessage.address = nullptr;
                        this->m_currentMessage.connection_id = 0;
                        this->m_currentMessage.length = 0;
                        this->m_uBytesWritten = 0;
                    } else if (this->m_currentMessage.length < this->m_uBytesWritten) {
                        TASQuake::Log("A bad happened\n");
                        abort();
                    }
                }
            }
            end:
            this->do_read();
        }
        else {
            // Connection terminated, exit
        }
    });
}

void ipc::client::send_message(void* data, uint32_t size) {
    if(!m_bConnected)
        return;
    write_to_socket(*socket_, &size, sizeof(uint32_t));
    write_to_socket(*socket_, data, size);
}

bool ipc::client::connect(const char* port) {
    try {
        this->io_service = new boost::asio::io_service();
        this->socket_ = new boost::asio::ip::tcp::socket(*this->io_service);
        boost::asio::ip::tcp::resolver resolver(*this->io_service);
        boost::asio::connect(*socket_, resolver.resolve({"127.0.0.1", port}));
        socket_->set_option(boost::asio::ip::tcp::no_delay(true));
        do_read();
        receiveThread = std::thread([&]() { io_service->run();});
        m_bConnected = true;
        return true;
    }
    catch (...) {
        TASQuake::Log("Unable to connect to port %s\n", port);
        return false;
    }
}


ipc::client::~client() {
    disconnect();
}

bool ipc::client::disconnect() {
    try {
        m_bConnected = false;
        if(io_service && !io_service->stopped()) {
            io_service->stop();
        }
        if(receiveThread.joinable())
            receiveThread.join();
        if(socket_ && socket_->is_open())
            socket_->close();
        return true;
    }
    catch (...) {
        return false;
    }
}
