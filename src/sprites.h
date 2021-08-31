#include "constants.h"

const unsigned char right_arrow_sprite[]={
                                          - 8,- 8,0x00,1,
                                          0,- 8,0x01,1,
                                          - 8,  0,0x00,1|OAM_FLIP_V,
                                          0,  0,0x01,1|OAM_FLIP_V,
                                          128
};

const unsigned char left_arrow_sprite[]={
                                         0,- 8,0x00,1|OAM_FLIP_H,
                                         - 8,- 8,0x01,1|OAM_FLIP_H,
                                         0,  0,0x00,1|OAM_FLIP_H|OAM_FLIP_V,
                                         - 8,  0,0x01,1|OAM_FLIP_H|OAM_FLIP_V,
                                         128
};
