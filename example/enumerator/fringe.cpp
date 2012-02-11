
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include "tree.h"

bool match_trees( tree_enumerator & te1, tree_enumerator & te2)
{
    std::string v1;
    std::string v2;

    bool res1 = te1.get( v1); 
    bool res2 = te2.get( v2); 

    if ( ! res1 && ! res2) return true;
    if ( ! res1 || ! res2) return false;
    if ( v1 == v2) return match_trees( te1, te2);
    else return false;
}

std::pair< node::ptr_t, node::ptr_t > create_eq_trees()
{
    branch::ptr_t tree1 = branch::create(
        leaf::create( "A"),
        branch::create(
            leaf::create( "B"),
            leaf::create( "C") ) );

    branch::ptr_t tree2 = branch::create(
        branch::create(
            leaf::create( "A"),
            leaf::create( "B") ),
        leaf::create( "C") );

    return std::make_pair( tree1, tree2);
}

std::pair< node::ptr_t, node::ptr_t > create_diff_trees()
{
    branch::ptr_t tree1 = branch::create(
        leaf::create( "A"),
        branch::create(
            leaf::create( "B"),
            leaf::create( "C") ) );

    branch::ptr_t tree2 = branch::create(
        branch::create(
            leaf::create( "A"),
            leaf::create( "X") ),
        leaf::create( "C") );

    return std::make_pair( tree1, tree2);
}

int main()
{
    {
        std::pair< node::ptr_t, node::ptr_t > pt = create_eq_trees();
        tree_enumerator te1( pt.first);
        tree_enumerator te2( pt.second);
        bool result = match_trees( te1, te2);
        std::cout << std::boolalpha << "eq. trees matched == " << result << std::endl;
    }
    {
        std::pair< node::ptr_t, node::ptr_t > pt = create_diff_trees();
        tree_enumerator te1( pt.first);
        tree_enumerator te2( pt.second);
        bool result = match_trees( te1, te2);
        std::cout << std::boolalpha << "diff. trees matched == " << result << std::endl;
    }

    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
