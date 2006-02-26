#include "cloud.h"
#include "behavior.h"
#include "cloud.anim.inl"

static void destroyCloud(Cloud* c)
{
    destroy(c->behavior);
}

Cloud* createCloud(int8 x, uint8 recess, uint8 speed, uint8 size)
{
    int16 height;

    Cloud* c = new(Cloud);
    c->destroy = destroyCloud;
    c->behavior = createBoring(speed);

    switch (size)
    {
        case CLOUD_TINY:
            c->animation = tinyAnimation();
            break;

        case CLOUD_SMALL:
            c->animation = smallAnimation();
            break;

        case CLOUD_MEDIUM:
            c->animation = mediumAnimation();
            break;

        case CLOUD_LARGE:
            c->animation = largeAnimation();
            break;
    }

    animationBeginning(c);
    
    height = entityHeight(c);
    c->position.x = x;
    c->position.y = -height - recess;

    return c;
}