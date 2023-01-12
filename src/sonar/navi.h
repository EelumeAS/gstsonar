#pragma once
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

//#define JOIN(x,y) x ## y
//#define ECHO(...) __VA_ARGS__
//#define static_assert(...) void static_assert_helper(const char [__COUNTER__]) {_Static_assert(__VA_ARGS__); }
#define static_assert(...)

inline uint64_t nanoseconds() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (uint64_t)ts.tv_sec * (uint64_t)1e9 + (uint64_t)ts.tv_nsec;
}

#pragma pack(4)

/** Struct for a single detection point in a type 1 (bathy) packet*/
typedef struct{
    uint32_t sample_number;         /**< Range. Detection point sample index (Distance from acc-centre is (ix*sound_velocity)/(2*Fs) */
    float    angle;                 /**< Angle. Detection point direction in radians */
    uint16_t upper_gate;            /**< Upper adaptive gate range. Same unit a sample_number*/
    uint16_t lower_gate;            /**< Lower adaptive gate range. Same unit a sample_number*/
    float intensity;                /**< Internsity. Backscatter return signal. Scaled to compensate for VGA and processing gain */
    uint16_t flags;                 /**< Bit0: Mag based detection | Bit1: Phase based detection | Bit2-8: Quality type | Bit9-12: Detection priority */
    uint8_t quality_flags;          /**< Bit0: SNr test pass | Bit1: Colinarity test pass */
    uint8_t quality_val;            /**< Ad-Hoc detection signal quality metric, based on detection strength and variance ovet time and swath */ 
}detectionpoint_t;
static_assert(sizeof(detectionpoint_t) == 20);

typedef enum // : uint32_t
{
  WBMS_BATH = 1,
  WBMS_FLS = 2,
} wbms_type_t;

/** COMMON HEADER, common packet header struct for all WBMS data packets*/
typedef struct{
    uint32_t preamble;              /**< Preable / Magic number, to simplify detection of a valid packet in a stream or blob of data. Allways 0xDEADBEEF */
    wbms_type_t type;               /**< WBMS packet type */
    uint32_t size;                  /**< Total packet size (including headers) in bytes */
    uint32_t version;               /**< WBMS packet version     */
    uint32_t RESERVED;              /**< 4 bytes reserved for furure use */
    uint32_t crc;                   /**< CRC32 of the size-24 bytes following the header  (crc32, zlib-style SEED=0x00000000, POLY=0xEDB88320)*/
}packet_header_t; //24 Bytes
static_assert(sizeof(packet_header_t) == 24);

/** BATH_DATA PACKET SUB-HEADER (WBMS Type 1 packet) */
typedef struct{
    float       snd_velocity;           /**< Filtered sanitized sound velocity in m/s*/
    float       sample_rate;            /**< Sample rate in reported range sample index, in Hz*/
    uint32_t    N;                      /**< Number of bathy points in packet*/
    uint32_t    ping_number;            /**< Ping number, increasing ping counter since sonar startup*/
    double      time;                   /**< Timestamp Unix time as fract (tx time)*/
    double      time_net;               /**< Timestamp Unix time as fract (send on network time)*/
    float       ping_rate;              /**< Set ping rate in Hz*/
    uint16_t    type;                   /**< WBMS Bathy data sub-type*/
    uint8_t     flags;                  /**< Beam distribution mode:  As defined in sonar_config.h.  
                                                Bit #0-3: 0=Undef, 1=512EA, 2=256EA, 3=256ED, 4=512EDx 5=256EDx 
                                                Bit #4-6: Multifreq index
                                                Bit #7: 0=NonAdaptiveRange 1=AdaptiveRange */
    uint8_t     sonar_mode;             /**< Sonar mode: As defined in sonar_config.h */
    float       time_uncertainty;       /**< For PPS/IRIG-B modes PPS jitter RMS, For pure NTP: NTP-offset*/
    uint16_t    multiping;              /**< Number of pings in multiping sequence*/
    uint16_t    multiping_seq_nr;       /**< Number of this ping in multiping sequence*/
    float       tx_angle;               /**< Tx beam steering in radians*/
    float       gain;                   /**< Intensity value gain*/
    float       tx_freq;                /**< Tx frequency [Hz]*/
    float       tx_bw;                  /**< Tx bandwidth [Hz]*/
    float       tx_len;                 /**< Tx length [s]*/
    float       snd_velocity_raw;       /**< Unfiltered sound velocity data from SVP probe*/
    float       tx_voltage;             /**< Tx voltage - peak-voltage signal over ceramics NaN for sonars without measurement */
    float       swath_dir;              /**< Center beam direction in radians */
    float       swath_open;             /**< Swath opening angle, edge to edge of swath in radians.*/
    float       gate_tilt;              /**< Gate tilt, in radians */
}bath_data_header_t; //88 bytes
static_assert(sizeof(bath_data_header_t) == 88);

#define BATH_DATA_MAX_DETECTION_POINTS    4096

/** BATHY PACKET, including common header, sub-header and payload */
typedef struct{
    packet_header_t         header;                                    //24B
    bath_data_header_t      sub_header;                                //88B
    //detectionpoint_t        dp[BATH_DATA_MAX_DETECTION_POINTS];        //20B*4k = 80kB
    //detectionpoint_t        dp[256];
    detectionpoint_t        dp[0];

    //int size() const {return sizeof(header) + sizeof(sub_header) + sizeof(detectionpoint_t) * sub_header.N;};
}bath_data_packet_t;


typedef struct{
    float       snd_velocity;           /**< Filtered sanitized sound velocity in m/s*/
    float       sample_rate;            /**< Sample rate in reported range sample index, in Hz*/
    uint32_t    N;                      /**< Number of bathy points in packet*/
    uint32_t    M;
    double      time;                   /**< Timestamp Unix time as fract (tx time)*/
    uint32_t dtype;
    int32_t t0;
    float       gain;                   /**< Intensity value gain*/
    uint32_t reserved_0;
    float       swath_dir;              /**< Center beam direction in radians */
    float       swath_open;             /**< Swath opening angle, edge to edge of swath in radians.*/
    float       tx_freq;                /**< Tx frequency [Hz]*/
    float       tx_bw;                  /**< Tx bandwidth [Hz]*/
    float       tx_len;                 /**< Tx length [s]*/
    uint32_t       tx_amp;
    uint32_t reserved_1[3];
    float       ping_rate;              /**< Set ping rate in Hz*/
    uint32_t    reserved_2;
    uint32_t ping_number;
    double time_net;
    uint32_t beams;
    int32_t vga_t1;
    int32_t vga_g1;
    int32_t vga_t2;
    int32_t vga_g2;
    uint32_t reserved_3;
    float       tx_angle;               /**< Tx beam steering in radians*/
    float       tx_voltage;             /**< Tx voltage - peak-voltage signal over ceramics NaN for sonars without measurement */
    uint8_t     flags;                  /**< Beam distribution mode:  As defined in sonar_config.h.  **/
    uint8_t     sonar_mode;             /**< Sonar mode: As defined in sonar_config.h
                                                Bit #0-3: 0=Undef, 1=512EA, 2=256EA, 3=256ED, 4=512EDx 5=256EDx 
                                                Bit #4-6: Multifreq index
                                                Bit #7: 0=NonAdaptiveRange 1=AdaptiveRange */
    uint16_t    reserved_4;
    float       gate_tilt;              /**< Gate tilt, in radians */
    uint32_t    reserved_5[8];
}fls_data_header_t;
static_assert(sizeof(fls_data_header_t) == 168);

#define N_BEAMS 128
typedef struct{
    packet_header_t         header;                                    //24B
    fls_data_header_t       sub_header;                                //168B
    char data[0];

    //uint16_t beam_intensity(int sample_index, int beam_index) const
    //{
    //    return reinterpret_cast<const uint16_t*>(data)[sample_index * sub_header.N + beam_index];
    //}

    //float beam_angle(int beam_index) const
    //{
    //    return reinterpret_cast<const float*>(reinterpret_cast<const uint16_t*>(data) + sub_header.M * sub_header.N)[beam_index];
    //}

    //int size() const
    //{
    //    return sizeof(header) + sizeof(sub_header) + sub_header.M * sub_header.N * sizeof(beam_intensity(0,0)) + sub_header.N * sizeof(beam_angle(0));
    //};

}fls_data_packet_t;

#pragma pack()
