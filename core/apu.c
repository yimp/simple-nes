#include "apu.h"
#include <stdlib.h>
#include <string.h>

#define NTSC_CPU_CLOCK      1789772
#define SAMPLE_RATE         44100
#define VIDEO_RATE          60
#define BYTES_PER_SAMPLE    4
#define SAMPLES_PER_FRAME   735
#define FRAME_COUNTER_RATE  240

static const u8 __duty_table[4] = {
    0x40, // 01000000 (12.5%)
    0x60, // 01100000 (25%)
    0x78, // 01111000 (50%)
    0x9F, // 10011111 (25% negated)
};

static const u8 __nes_length_table[] = {
	10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
	12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static float __pulse_table[] = {
    0.0,
    95.88 / ((8128.0 /  1) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  2) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  3) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  4) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  5) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  6) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  7) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  8) + 100.0) * 0x8000,
    95.88 / ((8128.0 /  9) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 10) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 11) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 12) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 13) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 14) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 15) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 16) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 17) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 18) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 19) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 20) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 21) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 22) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 23) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 24) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 25) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 26) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 27) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 28) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 29) + 100.0) * 0x8000,
    95.88 / ((8128.0 / 30) + 100.0) * 0x8000,
};

const float __k0 = (float)(NTSC_CPU_CLOCK) / (float)SAMPLE_RATE;                // sequencer
const float __k1 = (float)(FRAME_COUNTER_RATE) / (float)SAMPLE_RATE;            // envelope, length counter
const float __k2 = (float)(FRAME_COUNTER_RATE >> 1) / (float)SAMPLE_RATE;       // sweep

typedef struct
{
// registers
union {  struct { 
// reg0;
struct { 
    u8 envelope_period  : 4;
    u8 constant_volume : 1;
    u8 loop_envelope : 1;
    u8 duty : 2;
};

// reg1;
struct { 
    u8 sweep_shift_count : 3;
    u8 sweep_negative : 1;
    u8 sweep_period : 3;
    u8 sweep_enabled : 1;
};

// reg2;
struct { 
    u8 timer_low;
};

// reg3;
struct { 
    u8 timer_high : 3;
    u8 length_counter_load : 5;
};
};

struct {
struct { 
    u8 envelope_volume  : 4;
    u8 reg0_bit4 : 1;
    u8 disable_length_counter : 1;
    u8 reg0_bit67 : 2;
};
    u8 reg1;
    u16 timer : 11;
    u16 reg3_bit34567 : 5;
};
    u8 reg[4];
};
    int enabled;
    int step;   // 16 steps per wave
    int volume;
    int sequencer_period;
    float length_counter;
    float sweep_counter;
    float envelope_counter;
    float sequencer_counter;
} square_t;

static square_t __square[2];
static u8 __buffer[SAMPLES_PER_FRAME * BYTES_PER_SAMPLE];

u8 apu_bus_read(u16 addr)
{
    sizeof(square_t);
    (void)addr;
    return 0;
}

void apu_bus_write(u16 addr, u8 data)
{
	int chan = (addr & 4) ? 1 : 0;
    switch (addr & 0xFF)
    {
    case 0: 
    case 4: {
        __square[chan].reg[0] = data;
        break;
    }
    case 1:
    case 5: {
        __square[chan].reg[1] = data;
        break;
    }
    case 2: 
    case 6: {
        __square[chan].reg[2] = data;
        if (__square[chan].enabled)
			__square[chan].sequencer_period = ((int)__square[chan].timer + 1);
        break;
    }
    case 3: 
    case 7: {
        __square[chan].reg[3] = data;
        __square[chan].length_counter = __nes_length_table[(data >> 3)];
        __square[chan].volume = 16;
        __square[chan].sequencer_period = (__square[chan].timer + 1);
        break;
    }
	case 0x15: {
        if (data & 0x01) {
            __square[0].enabled = 1;
        }
		else {
            __square[0].enabled = 0;
            __square[0].length_counter = 0;
		}

        if (data & 0x02) {
            __square[1].enabled = 1;
        }
		else {
            __square[1].enabled = 0;
            __square[1].length_counter = 0;
		}
    }
    default:
        break;
    }
}

int square_render(square_t* square)
{
    if (!square->enabled) 
        return 0;

    square->envelope_counter -= __k1;
    while (square->envelope_counter < 0)
    {
        square->envelope_counter += ((int)square->envelope_period + 1);
        if (square->loop_envelope)
            square->volume = (square->volume - 1) & 0xF;
        else if (square->volume > 0)
            --square->volume;
    }

    if (square->length_counter > 0 && !square->disable_length_counter)
        square->length_counter -= __k1;

    if (!(square->length_counter > 0)) {
        return 0;
    }

    if (square->sweep_enabled && square->sweep_shift_count) {
        square->sweep_counter -= __k2;
        while (square->sweep_counter < 0) {
            square->sweep_counter += square->sweep_period + 1;
            if (square->sweep_negative) {
                square->sequencer_period -= (square->sequencer_period >> square->sweep_shift_count);
            } else {
                square->sequencer_period += (square->sequencer_period >> square->sweep_shift_count);
            }
        }
    }

    if ((square->sequencer_period) > 0x7FF || (square->sequencer_period) < 8)
    {
        return 0;
    }

    square->sequencer_counter -= __k0;
    while (square->sequencer_counter < 0) {
        square->sequencer_counter += (square->sequencer_period);
        square->step = (square->step + 1) & 0xF; 
    }

    int s = square->constant_volume ? square->envelope_volume : (square->volume);
    s &= 0xF;
    return (__duty_table[square->duty] & (1 << (square->step >> 1))) > 0
        ? s : -s;
}


sample_vec apu_end_frame()
{
    i16* buffer = (i16*)__buffer;
    for (int i = 0; i < SAMPLES_PER_FRAME; ++i) 
    {
        int s0 = square_render(&__square[0]);
        int s1 = square_render(&__square[1]);
        s0 = s0 > 0 ? s0 : 0;
        s1 = s1 > 0 ? s1 : 0;
        buffer[i << 1] = (i16)(__pulse_table[s0 + s1]);
        buffer[(i << 1) | 1] = buffer[i << 1];
    }
    sample_vec data = {
        .size = SAMPLES_PER_FRAME,
        .data = buffer,
    };
    return data;
}   

void apu_reset()
{
    memset(__square, 0x00, sizeof(__square));
    memset(__buffer, 0x00, sizeof(__buffer));
}

