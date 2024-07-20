#include <raylib.h>
#include <assert.h>


int main(void)
{
    Image heart = LoadImage("./assets/atlas.png");
    assert(ExportImageAsCode(heart, "./assets/atlas.h"));
    Image img;
    return 0;
}
