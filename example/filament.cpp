//          Copyright Oliver Kowalke 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>

#include <unwind.h>

#include <boost/context/fiber_context.hpp>

namespace ctx = boost::context;

class filament {
private:
    std::queue< filament >  &   q_;
    ctx::fiber_context          f_;

public:
    filament( std::queue< filament > & q) :
        q_{ q },
        f_{} {
    }

    template< typename Fn >
    filament( std::queue< filament > & q, Fn && fn) :
        q_{ q },
        f_{ [this,fn=std::forward<Fn>(fn)](ctx::fiber_context &&)->ctx::fiber_context{
            fn( [this](){
                    // deque next filament
                    filament next = std::move( q_.front() );
                    q_.pop();
                    // queue this filament
                    q_.push( std::move( * this) );
                    // resume next filament
                    q_.back().resume_next( next);
                });
            return {};
        }} {
    }

    ~filament() {
        if ( f_) {
            // unwind the filament's stack
            std::move( f_).resume_with( ctx::unwind_fiber);
        }
    }

    filament( filament const&) = delete;
    filament & operator=( filament const&) = delete;

    filament( filament && other):
        q_{ other.q_ },
        f_{} {
        other.f_.swap( f_);
    }

    filament & operator=( filament && other) {
        if ( this != & other) {
            other.q_.swap( q_);
            other.f_.swap( f_);
        }
        return * this;
    }

    void resume_next( filament & fila) {
        std::move( fila.f_).resume_with([this](ctx::fiber_context&& f)->ctx::fiber_context {
            // store filament's suspended fiber_context
            f_ = std::move( f);
            return {};
        });
    }

};

int main( int argc, char * argv[]) {
    std::queue< filament > q;
    filament main{ q };
    unsigned int count = 3;
    filament f1{
        q,
        [count,&q,&main](std::function<void()> && next){
            for ( unsigned int i = 1; i <= count; ++i) {
                std::printf("ping\n");
                if ( i == count) {
                    // queue the filament `main` to 
                    // resume it at the end of the loop
                    q.push( std::move( main) );
                }
                // resume next filament
                next();
            }
        }};
    filament f2{
        q,
        [count](std::function<void()> && next){
            for ( unsigned int i = 1; i <= count; ++i) {
                std::printf("pong\n");
                // resume next filament
                next();
            }
        }};
    // queue filament `f2`
    q.push( std::move( f2) );
    // resume filament `f1`
    main.resume_next( f1);
    return EXIT_SUCCESS;
}
