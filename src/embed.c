#include <raylib.h>
#include <assert.h>


int main(void)
{
    Image puzzle_atlas = LoadImage("./assets/atlas.png");
    assert(ExportImageAsCode(puzzle_atlas, "./assets/atlas.h"));

    Image world_atlas = LoadImage("./assets/world_atlas.png");
    assert(ExportImageAsCode(world_atlas, "./assets/world_atlas.h"));

    Image player_atlas = LoadImage("./assets/player_atlas.png");
    assert(ExportImageAsCode(player_atlas, "./assets/player_atlas.h"));
    return 0;
}
