
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ENUMERATOR_H
#define ENUMERATOR_H

#include <boost/bind.hpp>
#include <boost/config.hpp>

#include <boost/context/all.hpp>

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
# endif

template< typename T >
class enumerator {
private:
    boost::contexts::context          ctx_;
    bool                                complete_;
    bool                                do_unwind_;

    static void run( enumerator * self)
    { self->enumerate(); }

protected:
    virtual void enumerate() = 0;

    void yield_return( T const& v)
    {
        intptr_t vp = reinterpret_cast< intptr_t >( & v);
        ctx_.suspend( vp);
    }

    void yield_break()
    {
        complete_ = true;
        ctx_.suspend();
    }

public:
    enumerator( bool do_unwind = true): 
        ctx_(
            & enumerator::run, this,
            boost::contexts::default_stacksize(),
			boost::contexts::no_stack_unwind, boost::contexts::return_to_caller),
        complete_( false),
        do_unwind_( do_unwind)
    {}

    ~enumerator()
    {
        if ( do_unwind_ && ! ctx_.is_complete() )
            ctx_.unwind_stack();
    }

    bool get( T & result)
    {
        intptr_t vp = 0;
        if ( ! ctx_.is_started() )
            vp = ctx_.start();
        else
            vp = ctx_.resume();
        if ( vp) result = * reinterpret_cast< T * >( vp);
        return ! ( complete_ || ctx_.is_complete() );
    }
};

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif


#endif // ENUMERATOR_H
