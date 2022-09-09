#include "how2thread.hpp"
#include <cstdio>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    size_t size;
    FILE *f;
    if(argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <fname> <mask>" << std::endl;
        return 1;
    }
    f = fopen(argv[1], "rb");
    if (!f) {
        printf("Error opening the input file.\n");
        printf("%s\n", argv[1]);
        return 1;
    }

    std::string mask(argv[2]); // add mask check?
    fseek(f, 0, SEEK_END);
    size = ftell(f);
#ifdef DEBUG_V
    printf("size: %u\n", size);
#endif
    std::vector<char> buf(size, 0);// preallocate zero string
    fseek(f, 0, SEEK_SET);
    size_t read = fread(buf.data(), 1, size, f);
    fclose(f);

    auto sc = how2thread::Scheduler(buf, mask);
#ifdef DEBUG_V
    for(auto &i: buf)
    {
        std::cout << i;
        std::flush(std::cout);
    }
    std::cout << std::endl;
    std::cout << "Request buffer size: " << sc.data_arr.size() << std::endl;
    std::flush(std::cout);
#endif
    for(auto i = sc.cbegin(); i < sc.cend(); ++i)
    {
        std::cout << i->line_no << " " << i->line_pos << " " << i->data << "\n";
    }

//    how2thread::Request req{ std::string("And bad mistakes ?"), 2, 2, true};
//
//    how2thread::Actor_processor a;
//    auto re = a.parse_request(req, "?ad");
//    std::cout << re.line_no << ": " << re.data << "\n";

    return 0;
}