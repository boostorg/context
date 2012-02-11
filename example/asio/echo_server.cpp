
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// adapted version of echo-server example using coroutines from 
// Chris Kohlhoff: http://blog.think-async.com/2009/08/secret-sauce-revealed.html 

#include <cstdlib>
#include <iostream>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>

#include "continuation.h"

class server : public boost::enable_shared_from_this< server >,
               private continuation
{
private:
    boost::asio::ip::tcp::acceptor      acceptor_;
    boost::system::error_code           ec_;
    std::size_t                         n_;

    void do_()
    {
       for (;;)
       {
            boost::asio::ip::tcp::socket socket( acceptor_.get_io_service() );
            acceptor_.async_accept( socket, boost::bind( & server::operator(), this->shared_from_this(), _1, 0) );
            suspend();

            while ( ! ec_)
            {
                boost::array< char, 1024 > buffer;

                socket.async_read_some(
                    boost::asio::buffer( buffer),
                    boost::bind( & server::operator(), this->shared_from_this(), _1, _2) ); 
                suspend();

                if ( ec_) break;

                boost::asio::async_write(
                    socket,
                    boost::asio::buffer( buffer, n_),
                    boost::bind( & server::operator(), this->shared_from_this(), _1, _2) ); 
                suspend();
            }
       }
    }

    server( boost::asio::io_service & io_service, short port) :
        continuation( boost::bind( & server::do_, this) ),
        acceptor_( io_service, boost::asio::ip::tcp::endpoint( boost::asio::ip::tcp::v4(), port) ),
        ec_(), n_( 0)
    {}

public:
    typedef boost::shared_ptr< server >     ptr_t;

    static ptr_t create( boost::asio::io_service & io_service, short port)
    { return ptr_t( new server( io_service, port) ); }

    void operator()( boost::system::error_code ec, size_t n)
    {
        ec_ = ec;
        n_ = n;
        resume();
    }
};

int main( int argc, char * argv[])
{
    try
    {
        if ( argc != 2)
        {
            std::cerr << "Usage: echo_server <port>\n";
            return 1;
        }
        {
            boost::asio::io_service io_service;
            io_service.post(
                boost::bind(
                    & server::operator(),
                    server::create( io_service, boost::lexical_cast< short >( argv[1]) ),
                    boost::system::error_code(), 0) );
            io_service.run();
        }
        std::cout << "Done" << std::endl;

        return  EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "Exception: " << e.what() << std::endl; }

    return EXIT_FAILURE;
}

