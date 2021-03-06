#include "LZ77.h"
#include "LZend.nuevo.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char** argv){
    LZparser* p;
    p = new LZEnd2(argv[1]);
    unsigned int f = p->parse();
    std::cout<<"Parsed to: "<<f<<" factors"<<std::endl;
    std::cout<<"Bits needed:"<<p->bits_size<<std::endl; 
    delete p;
    return EXIT_SUCCESS;
}
