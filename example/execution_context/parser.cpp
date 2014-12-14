
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>

#include <boost/context/execution_context.hpp>
#include <boost/context/fixedsize.hpp>

/*
 * grammar:
 *   P ---> E '\0'
 *   E ---> T {('+'|'-') T}
 *   T ---> S {('*'|'/') S}
 *   S ---> digit | '(' E ')'
 */
class Parser{
   char next;
   std::istream& is;
   std::function<void(char)> cb;

   char pull(){
        return std::char_traits<char>::to_char_type(is.get());
   }

   void scan(){
       do{
           next=pull();
       }
       while(isspace(next));
   }

public:
   Parser(std::istream& is_,std::function<void(char)> cb_) :
      next(), is(is_), cb(cb_)
    {}

   void run() {
      scan();
      E();
   }

private:
   void E(){
      T();
      while (next=='+'||next=='-'){
         cb(next); 
         scan();
         T();
      }
   }

   void T(){
      S();
      while (next=='*'||next=='/'){
         cb(next); 
         scan();
         S();
      }
   }

   void S(){
      if (std::isdigit(next)){
         cb(next);
         scan();
      }
      else if(next=='('){
         cb(next); 
         scan();
         E();
         if (next==')'){
             cb(next); 
             scan();
         }else{
             exit(2);
         }
      }
      else{
         exit(3);
      }
   }
};

void foo() {
    std::istringstream is("1+1");
    bool done=false;
    char c;
    boost::context::execution_context main_ctx(
        boost::context::execution_context::current() );
    // invert control flow
    boost::context::execution_context parser_ctx(
        boost::context::fixedsize(),
        [&is,&main_ctx,&done,&c](){
            Parser p(is,[&main_ctx,&c](char ch){
                c=ch;
                main_ctx.jump_to();
            });
            p.run();
            done=true;
            main_ctx.jump_to();
        });

    // user-code pulls parsed data from parser
    while(!done){
        parser_ctx.jump_to();
        printf("Parsed: %c\n",c);
    }
}

int main() {
    std::thread t( foo);
    t.join();
    return 0;
}
