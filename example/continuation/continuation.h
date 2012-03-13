
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef CONTINUATION_H
#define CONTINUATION_H

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/context/all.hpp>
#include <boost/function.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
# endif

class continuation
{
private:
    boost::contexts::context                  ctx_;
    boost::function< void( continuation &) >    fn_;

    void trampoline_()
    { fn_( * this); }

public:
    continuation( boost::function< void( continuation &) > const& fn) :
        ctx_(
            & continuation::trampoline_, this,
            boost::contexts::default_stacksize(),
			boost::contexts::no_stack_unwind, boost::contexts::return_to_caller),
        fn_( fn)
    {}

    void resume()
    {
        if ( ! ctx_.is_started() ) ctx_.start();
        else ctx_.resume();
    }

    void suspend()
    { ctx_.suspend(); }

    void join( boost::function< void( continuation &) > const& fn)
    {
        continuation c( fn);
        while ( ! c.is_complete() )
            c.resume();
    }

    bool is_complete() const
    { return ctx_.is_complete(); }
};

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#endif // CONTINUATION_H
