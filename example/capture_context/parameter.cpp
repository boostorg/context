
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include <boost/context/all.hpp>
#include <boost/lexical_cast.hpp>

namespace ctx = boost::context;

class X{
private:
    std::exception_ptr excptr_;
    ctx::captured_context ctx_;

public:
    X():
        excptr_(),
        ctx_(
             [=](ctx::captured_context ctx, void * vp){
                try {
                    for (;;) {
                        int i = * static_cast< int * >( vp);
                        std::string str = boost::lexical_cast<std::string>(i);
                        auto result = ctx( & str);
                        ctx = std::move( std::get<0>( result) );
                        vp = std::get<1>( result);
                    }
                } catch ( ctx::detail::forced_unwind const&) {
                    throw;
                } catch (...) {
                    excptr_=std::current_exception();
                }
                return ctx;
             })
    {}

    std::string operator()(int i){
        auto result = ctx_( & i);
        ctx_ = std::move( std::get<0>( result) );
        void * ret = std::get<1>( result);
        if(excptr_){
            std::rethrow_exception(excptr_);
        }
        return * static_cast< std::string * >( ret);
    }
};

int main() {
    X x;
    std::cout<<x(7)<<std::endl;
    std::cout << "done" << std::endl;
}
