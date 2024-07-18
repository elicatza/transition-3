#include <raylib.h>
#include <assert.h>


int main(void)
{
    Image heart = LoadImage("./assets/heart.png");
    assert(ExportImageAsCode(heart, "./assets/heart.h"));
    Image img;
    return 0;
}
