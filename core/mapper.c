#include "mapper.h"
#include <string.h>
#include <stdlib.h>

void nrom_init(mapper_t* mapper);
void uxrom_init(nes_rom_info* context, mapper_t* mapper);
void uxrom_destroy(nes_rom_info* context, mapper_t* mapper);

void mmc3_init(nes_rom_info* context, mapper_t* mapper);
void mapper246_init(nes_rom_info* context, mapper_t* mapper);

void mapper_init(nes_rom_info* context, mapper_t* mapper)
{
    memset(mapper, 0x00, sizeof(mapper_t));
    switch (context->mapper_id)
    {
    case 0:
        nrom_init(mapper);
        break;
    case 2:
        uxrom_init(context, mapper);
        break;
    case 4:
        mmc3_init(context, mapper);
        break;
    case 246:
        mapper246_init(context, mapper);
        break;
    default:
        break;
    }
}

void mapper_destroy(nes_rom_info* context, mapper_t** pmapper)
{
    mapper_t* mapper = *pmapper;

    if (!pmapper || !context || !(*pmapper))
        return;

    mapper = *pmapper;
    switch (context->mapper_id)
    {
    case 2: {
        uxrom_destroy(context, mapper);
        break;
    }
    default: break;
    }

    if (mapper->priv)
    {
        free(mapper->priv);
    }

    free(mapper);
    *pmapper = NULL;
}


