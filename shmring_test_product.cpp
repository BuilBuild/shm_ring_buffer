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
    RingBufferShm<MSG> p(filePath, 128, Product);
    MSG msg{};
    strcpy(msg.name, "LeiJiulong");
    for (int i = 0; i <= 135; ++i)
    {
        msg.age = i;
        p.enQueue(msg);
    }
    for (int i = 0; i < 20; i++)
    {
        p.getQueue();
    }
    getchar();
    return 0;
}