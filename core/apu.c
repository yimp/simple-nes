#include "apu.h"
#include "bus.h"
#include <stdlib.h>
#include <string.h>

#pragma region "constants and lookup tables"
#define NTSC_CPU_CLOCK      1789772
#define SAMPLE_RATE         44100
#define VIDEO_RATE          60
#define BYTES_PER_SAMPLE    4
#define SAMPLES_PER_FRAME   735
#define FRAME_COUNTER_RATE  240

static const u8 __duty_table[] = {
    0x40, // 01000000 (12.5%)
    0x60, // 01100000 (25%)
    0x78, // 01111000 (50%)
    0x9F, // 10011111 (25% negated)
};

static const u8 __nes_length_table[] = {
	10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
	12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const int __ntsc_noise_period[] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

static const int __ntsc_dmc_period[] = {
   428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
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

static float __tnd_table[16][16][128];

const float __k0 = (float)(NTSC_CPU_CLOCK) / (float)SAMPLE_RATE;                // sequencer
const float __k1 = (float)(FRAME_COUNTER_RATE) / (float)SAMPLE_RATE;            // envelope, length counter
const float __k2 = (float)(FRAME_COUNTER_RATE >> 1) / (float)SAMPLE_RATE;       // sweep

#pragma endregion

#pragma region "context"

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


typedef struct {
// registers
union {  struct { 
// reg0;
struct { 
    u8 linear_counter_load : 7;
    u8 length_counter_halt  : 1;
};

// reg1;
struct { 
    u8 unused;
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
    u8 reg0_bit0123456 : 7;
    u8 length_counter_control : 1;
};
    u8 reg1;
    u16 timer : 11;
    u16 reg3_bit34567 : 5;
};
    u8 reg[4];
};
    int enabled;
    int step;   // 32 steps per wave
    int sequencer_period;
    float length_counter;
    float sequencer_counter;
    float linear_length_counter;
	bool linear_reload;
} triangle_t;


typedef struct {
// registers
union {  struct { 
// reg0;
struct { 
    u8 envelope_period  : 4;
    u8 constant_volume : 1;
    u8 envelope_loop : 1;
    u8 reg0_bit67 : 2;
};
// reg1;
struct { 
    u8 reg1_bit01234567;
};

// reg2;
struct { 
    u8 noise_period : 4;
    u8 reg2_bit456 : 3;
    u8 noise_mode : 1;
};

// reg3;
struct { 
    u8 reg3_bit012 : 3;
    u8 length_counter_load : 5; 
};
};

struct {
struct { 
    u8 envelope_volume : 4;
    u8 reg3_bit4 : 1;
    u8 length_counter_halt : 1;
    u8 reg0_bit67_ : 1;
};
    u8 reg1;
    u8 reg2;
    u8 reg3;
};
    u8 reg[4];
};
	u16 lfsr;
    int enabled;
    int volume;
    int sequencer_period;
    float length_counter;
    float sequencer_counter;
    float envelope_counter;
} noise_t;

typedef struct {
// registers
union {  struct { 
// reg0;
struct { 
    u8 frequency  : 4;
    u8 reg0_bit45 : 2;
    u8 loop : 1;
    u8 irq : 1;
};
// reg1;
struct { 
    u8 load_counter : 7;
    u8 reg1_bit7 : 1;
};

// reg2;
struct { 
    u8 sample_address;
};

// reg3;
struct { 
    u8 sample_length;
};
};
    u8 reg[4];
};
    int enabled;
    int volume;
    int length;
    int address;
    int cur_byte;
    int cur_bit;
    float sequencer_counter;
} dmc_t;

static square_t     __square[2];
static triangle_t   __triangle;
static noise_t      __noise;
static dmc_t        __dmc;
static u8 __buffer[SAMPLES_PER_FRAME * BYTES_PER_SAMPLE];

#pragma endregion

#pragma region "helper functions"
#define BIT(x, n) (((x) >> (n)) & 1)
static inline void update_lfsr(noise_t *chan)
{
	chan->lfsr |= (BIT(chan->lfsr, 0) ^ BIT(chan->lfsr, (chan->noise_mode) ? 6 : 1)) << 15;
	chan->lfsr >>= 1;
}

static void dmc_reset(dmc_t *chan)
{
	chan->address = 0xC000 + (u16)(chan->sample_address << 6);
	chan->length = (u16)(chan->sample_length << 4) + 1;
	chan->cur_bit = 0x08;
	chan->enabled = true;
}
#pragma endregion

#pragma region "bus I/O"

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
    case 8: {
        __triangle.reg[0] = data;
        if (__triangle.enabled)
		{
            __triangle.linear_length_counter = __triangle.linear_counter_load;
		}
        break;
    }
    case 9: {
        __triangle.reg[1] = data;
        break;
    }
    case 0xA: {
        __triangle.reg[2] = data;
        __triangle.sequencer_period = (__triangle.timer + 1);
        break;
    }
    case 0xB: {
        __triangle.reg[3] = data;
        __triangle.sequencer_period = (__triangle.timer + 1);
        if (__triangle.enabled)
        {
            __triangle.length_counter = __nes_length_table[__triangle.length_counter_load];
            __triangle.linear_length_counter = __triangle.linear_counter_load;
            __triangle.linear_reload = true;
        }
        break;
    }
    case 0xC: {
        __noise.reg[0] = data; break;
    }
    case 0xD: {
        __noise.reg[1] = data; break;
    }
    case 0xE: {
        __noise.reg[2] = data; break;
    }
    case 0xF: {
        __noise.reg[3] = data;
        if (__noise.enabled)
		{
            __noise.length_counter = __nes_length_table[__noise.length_counter_load];
            __noise.volume = 16;            
		}
		break;
    }
    case 0x10: {
        __dmc.reg[0] = data;
        if (!(data & 0x80)) {
			/* we dont need irq for now */
		}
        break;
    }
    case 0x11: {
        __dmc.reg[1] = data;
        __dmc.volume = __dmc.load_counter;
		break;
    }    
    case 0x12: { 
        __dmc.reg[2] = data; break;
    }
    case 0x13: {
        __dmc.reg[3] = data; break;
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

        if (data & 0x04)
			__triangle.enabled = true;
		else
		{
			__triangle.enabled = false;
			__triangle.length_counter = 0;
			__triangle.linear_length_counter = 0;
		}

        if (data & 0x08)
			__noise.enabled = true;
		else
		{
			__noise.enabled = false;
            __noise.length_counter = 0;
		}

        if (data & 0x10)
		{
			if (!__dmc.enabled)
			{
				__dmc.enabled = true;
				dmc_reset(&__dmc);
			}
		}
		else
			__dmc.enabled = false;
        break;
    }
    case 0x17: {
        /* we dont need irq for now */
        break;
    }
    default: break;
    }
}

#pragma endregion

#pragma region "channels"
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
    return (__duty_table[square->duty] & (1 << (7 ^ (square->step >> 1)))) > 0
        ? s : -s;
}

int triangle_render(triangle_t* triangle)
{
    static int output = 0;
    if (! triangle->enabled)
	{
		return output = 0;
	}

    if (triangle->linear_reload)
        triangle->linear_length_counter = triangle->linear_counter_load;
    else if (triangle->linear_length_counter > 0)
        triangle->linear_length_counter -= __k1;

    if (!triangle->length_counter_halt)
        triangle->linear_reload = false;

    if (triangle->length_counter && !triangle->length_counter_halt)
        triangle->length_counter -= __k1;

	if (!(triangle->linear_length_counter > 0 && triangle->length_counter > 0))
	{
		return output = 0;
	}

	if (triangle->sequencer_period < 4)
	{
		return output = 0;
	}

	triangle->sequencer_counter -= __k0;
	while (triangle->sequencer_counter < 0)
	{
		triangle->sequencer_counter += triangle->sequencer_period;
		triangle->step = (triangle->step + 1) & 0x1f;
		output  = (triangle->step & 0x0f) ^ (0x10 - (bool)(triangle->step & 0x10));
        output &= 0x0F;
	}
    return output;
}

int noise_render(noise_t* noise)
{
    if (!noise->enabled)
	{
		return 0;
	}

    noise->envelope_counter -= __k1;
    while (noise->envelope_counter < 0)
    {
        noise->envelope_counter += ((int)noise->envelope_period + 1);
        if (noise->envelope_loop)
            noise->volume = (noise->volume - 1) & 0xF;
        else if (noise->volume > 0)
            --noise->volume;
    }

    if (!noise->length_counter_halt)
    {
        if (noise->length_counter > 0) {
            noise->length_counter -= __k1;
        }
    }

    if (!(noise->length_counter > 0)) 
    {
		return 0;
    }

    noise->sequencer_counter -= __k0;
	while (noise->sequencer_counter < 0)
	{
		noise->sequencer_counter += __ntsc_noise_period[noise->noise_period];
        update_lfsr(noise);
	}

    int output = noise->constant_volume ? noise->envelope_volume : noise->volume;
	return (noise->lfsr & 1) ? output : -output;
}

int dmc_render(dmc_t* dmc)
{
    if (!dmc->enabled) {
        goto EXIT;
    }

    dmc->sequencer_counter -= __k0;
    while (dmc->sequencer_counter < 0) {
        dmc->sequencer_counter += __ntsc_dmc_period[dmc->frequency];
        if (!dmc->length)
        {
            dmc->enabled = false;
            if (dmc->loop)
                dmc_reset(dmc);
            else
            {
                /* we dont need irq for now */
                break;
            }
        }

        if (dmc->cur_bit & 0x8)
        {
            dmc->cur_bit = 0;
            dmc->cur_byte = bus_read(dmc->address++);
            dmc->address >>= (bool)(dmc->address & 0x10000);
            dmc->length--;
        }
        
        int delta = (bool)(dmc->cur_byte & (1 << dmc->cur_bit));
        dmc->volume += (delta << 2) - 2;
        ++dmc->cur_bit;
    }
EXIT:
    if (dmc->volume < 0)
        dmc->volume = 0;
    dmc->volume &= 0x7F;
    return dmc->volume;
}

#pragma endregion

#pragma region "basics"
sample_vec apu_end_frame()
{
    i16* buffer = (i16*)__buffer;
    for (int i = 0; i < SAMPLES_PER_FRAME; ++i) 
    {
        int s0 = square_render(&__square[0]);
        int s1 = square_render(&__square[1]);
        int t = triangle_render(&__triangle);
        int n = noise_render(&__noise);
        int d = dmc_render(&__dmc);
        s0 = s0 > 0 ? s0 : 0;
        s1 = s1 > 0 ? s1 : 0;
        n = n > 0 ? n : 0;
        // s0 = s1 = n = t = 0;
        buffer[i << 1] = (i16)(__pulse_table[s0 + s1]) + __tnd_table[t][n][d];
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
    memset(&__triangle, 0x00, sizeof(__triangle));
    memset(&__noise, 0x00, sizeof(__noise));
    memset(&__dmc, 0x00, sizeof(__dmc));
    memset(__buffer, 0x00, sizeof(__buffer));

    for (int t = 0; t < 16; t++)
	{
		for (int n = 0; n < 16; n++)
		{
			for (int d = 0; d < 128; d++)
			{
				float tnd_out = (t / 8227.0f) + (n / 12241.0f) + (d / 22638.0f);
				tnd_out = (tnd_out == 0.0f) ? 0.0f : 159.79f / ((1.0f / tnd_out) + 100.0f);
				__tnd_table[t][n][d] = (tnd_out * 0x8000);
			}
		}
	}
    __noise.lfsr = 1;
}

#pragma endregion