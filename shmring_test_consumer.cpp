#include <string>
#include "shmring.hpp"

typedef struct
{
    char name[128];
    int age;

} MSG;

int main()
{
    std::string filePath = "/home/leijiulong/桌面/shmringbuffer/shmring.hpp";
    RingBufferShm<MSG> p(filePath, 128, Consumer);
    for (int i = 0; i < 20; i++)
    {
        MSG a = p.getQueue();
        std::cout << "name: " << a.name << "  age:  " << a.age << std::endl;
    }
    getchar();
    return 0;
}