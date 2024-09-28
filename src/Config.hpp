#pragma once

#include <set>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <functional>
#include <libconfig.h++>
#include <sys/time.h>
#include <any>
#include <mutex>

//~65k
#define ENABLE_LOG_DEBUG

// Some more debug output not usefull for users (Developer Debug)
#define DDEBUG

// enable audio support
#define AUDIO_SUPPORT
// enable audio processing library
#define LIB_AUDIO_PROCESSING

// disable tunings (debugging)
// #define NO_TUNINGS

#define IMP_AUTO_VALUE 16384
#define OSD_AUTO_VALUE 16384
#define IVS_AUTO_VALUE 16384

#define THREAD_SLEEP 25000
#define GET_STREAM_BLOCKING false

#if defined(PLATFORM_T31)
    #define DEFAULT_ENC_MODE_0 "FIXQP"
    #define DEFAULT_ENC_MODE_1 "CAPPED_QUALITY"
    #define DEFAULT_BUFFERS_0 4
    #define DEFAULT_BUFFERS_1 2
    #define DEFAULT_SINTER 128
    #define DEFAULT_TEMPER 128
    #define DEFAULT_SINTER_VALIDATE validateInt255
    #define DEFAULT_TEMPER_VALIDATE validateInt255
#elif defined(PLATFORM_T23)
    #define DEFAULT_ENC_MODE_0 "SMART"
    #define DEFAULT_ENC_MODE_1 "SMART"
    #define DEFAULT_BUFFERS_0 2
    #define DEFAULT_BUFFERS_1 2
    #define DEFAULT_SINTER 128
    #define DEFAULT_TEMPER 128
    #define DEFAULT_SINTER_VALIDATE validateInt255
    #define DEFAULT_TEMPER_VALIDATE validateInt255
#else
    #define DEFAULT_ENC_MODE_0 "SMART"
    #define DEFAULT_ENC_MODE_1 "SMART"
    #define DEFAULT_BUFFERS_0 2
    #define DEFAULT_BUFFERS_1 2
    #define DEFAULT_SINTER 50
    #define DEFAULT_TEMPER 50
    #define DEFAULT_SINTER_VALIDATE validateInt50_150
    #define DEFAULT_TEMPER_VALIDATE validateInt50_150
#endif

#define OSD_STREAMS 2

extern std::mutex mutex_main;

struct OsdConfigItem
{
    bool *streams; //[OSD_STREAMS];
    int *posX;
    int *posY;
    int width;
    int height;
    int *transparency;
    int *rotation;
    char *text;
    char *file;
    char *name;
    //int flags;

    OsdConfigItem()
        : streams(nullptr), posX(nullptr), posY(nullptr), width(0), height(0),
          transparency(nullptr), rotation(nullptr), text(nullptr), file(nullptr), name(nullptr)//, flags(0)
    {
        /*
        for (int i = 0; i < OSD_STREAMS; ++i)
        {
            streams[i] = false;
        }
        */
    };

    ~OsdConfigItem()
    {
        delete[] streams;
        delete posX;
        delete posY;
        delete transparency;
        delete rotation;
        delete[] text;
        delete[] file;
        delete[] name;
    }

    void assign_or_update(const OsdConfigItem *osdConfigItem)
    {
        if(osdConfigItem->streams != nullptr)
        {
            if (streams != nullptr) 
                delete[] streams;            

            streams = new bool[OSD_STREAMS];
            for (int i = 0; i < OSD_STREAMS; ++i)
            {
                streams[i] = osdConfigItem->streams[i];
            }
        }
        else if(streams == nullptr)
        {
            streams = new bool[OSD_STREAMS];
        }

        if(osdConfigItem->posX != nullptr)
        {
            if (posX != nullptr) 
                delete posX;

            posX = new int;
            *posX = *(osdConfigItem->posX);  
        }
        else if(posX == nullptr)
        {
            posX = new int{0};
        }

        if(osdConfigItem->posY != nullptr)
        {
            if (posY != nullptr) 
                delete posY;

            posY = new int;
            *posY = *(osdConfigItem->posY);  
        }
        else if(posY == nullptr)
        {
            posY = new int{0};
        }

        if(osdConfigItem->transparency != nullptr)
        {
            if (transparency != nullptr) 
                delete transparency;

            transparency = new int;
            *transparency = *(osdConfigItem->transparency);  
        }
        else if(transparency == nullptr)
        {
            transparency = new int{255};
        }

        if(osdConfigItem->rotation != nullptr)
        {
            if (rotation != nullptr) 
                delete rotation;

            rotation = new int;
            *rotation = *(osdConfigItem->rotation);  
        }
        else if(rotation == nullptr)
        {
            rotation = new int{0};
        }

        if(osdConfigItem->text != nullptr)
        {
            if(text != nullptr)
                delete[] text;
            
            text = new char[strlen(osdConfigItem->text) + 1];
            strcpy(text, osdConfigItem->text);
        }

        if(osdConfigItem->name != nullptr)
        {
            if(name != nullptr) 
                delete[] name;

            name = new char[strlen(osdConfigItem->name) + 1];
            strcpy(name, osdConfigItem->name);
        }

        if (osdConfigItem->file != nullptr) 
        {
            const char *delimiter = ":";
            if (strchr(osdConfigItem->file, *delimiter))
            {
                if(file != nullptr) 
                    delete[] file;

                char *token;
                char *tmpFile = strdup(osdConfigItem->file);

                token = strtok_r(tmpFile, delimiter, &tmpFile);
                if (token != nullptr)
                {
                    file = new char[strlen(token) + 1];
                    strcpy(file, token);
                }

                if (strchr(tmpFile, *delimiter))
                {
                    token = strtok_r(nullptr, delimiter, &tmpFile);
                    if (token != nullptr)
                        width = atoi(token);
                    else
                        width = 0;
                }

                if (!strchr(tmpFile, *delimiter))
                {
                    token = strtok_r(nullptr, delimiter, &tmpFile);
                    if (token != nullptr)
                        height = atoi(token);
                    else
                        height = 0;
                }

                free(tmpFile);
            }
            else
            {
                if(file != nullptr) 
                    delete[] file;
                
                file = new char[strlen(osdConfigItem->file) + 1];
                strcpy(file, osdConfigItem->file);

                width = osdConfigItem->width;
                height = osdConfigItem->height;
            }       
        }
    };
};

struct roi
{
    int p0_x;
    int p0_y;
    int p1_x;
    int p1_y;
};

template <typename T>
struct ConfigItem
{
    const char *path;
    T &value;
    T defaultValue;
    std::function<bool(const T &)> validate;
    bool noSave = false;
    const char *procPath = nullptr;
};

struct _stream_stats
{
    uint32_t bps;
    uint8_t fps;
    struct timeval ts;
};

struct _general
{
    const char *loglevel;
    int osd_pool_size;
    int imp_polling_timeout;
};

struct _rtsp
{
    int port;
    int est_bitrate;
    int out_buffer_size;
    int send_buffer_size;
    bool auth_required;
    const char *username;
    const char *password;
    const char *name;
};
struct _sensor
{
    int fps;
    int width;
    int height;
    const char *model;
    unsigned int i2c_address;
};
struct _image
{
    int contrast;
    int sharpness;
    int saturation;
    int brightness;
    int hue;
    int sinter_strength;
    int temper_strength;
    bool vflip;
    bool hflip;
    int running_mode;
    int anti_flicker;
    int ae_compensation;
    int dpc_strength;
    int defog_strength;
    int drc_strength;
    int highlight_depress;
    int backlight_compensation;
    int max_again;
    int max_dgain;
    int core_wb_mode;
    int wb_rgain;
    int wb_bgain;
};
#if defined(AUDIO_SUPPORT)
struct _audio
{
    bool input_enabled;
    const char *input_format;
    int input_vol;
    int input_bitrate;
    int input_gain;
    int input_sample_rate;
#if defined(LIB_AUDIO_PROCESSING)
    int input_alc_gain;
    int input_noise_suppression;
    bool input_high_pass_filter;
    bool input_agc_enabled;
    int input_agc_target_level_dbfs;
    int input_agc_compression_gain_db;
#endif
};
#endif
struct _osd
{
    int font_size;
    int font_stroke_size;
    int font_xscale;
    int font_yscale;
    int font_stroke;
    int font_yoffset;
    int start_delay;
    bool enabled;
    bool font_stroke_enabled;
    const char *font_path;
    const char *time_format;
    const char *uptime_format;
    unsigned int font_color;
    unsigned int font_stroke_color;
    _stream_stats stats;
    std::atomic<int> thread_signal;
};
struct _stream
{
    int gop;
    int max_gop;
    int fps;
    int buffers;
    int width;
    int height;
    int profile;
    int bitrate;
    int rotation;
    int scale_width;
    int scale_height;
    bool enabled;
    bool scale_enabled;
    bool power_saving;
    bool allow_shared;
    const char *mode;
    const char *rtsp_endpoint;
    const char *format{"JPEG"};
    /* JPEG stream*/
    int jpeg_quality;
    int jpeg_refresh;
    int jpeg_channel;
    int jpeg_idle_fps;
    const char *jpeg_path;
    _osd osd;
    _stream_stats stats;
#if defined(AUDIO_SUPPORT)
    bool audio_enabled;
#endif
};
struct _motion
{
    int monitor_stream;
    int debounce_time;
    int post_time;
    int cooldown_time;
    int init_time;
    int ivs_polling_timeout;
    int sensitivity;
    int skip_frame_count;
    int frame_width;
    int frame_height;
    int roi_0_x;
    int roi_0_y;
    int roi_1_x;
    int roi_1_y;
    int roi_count;
    bool enabled;
    const char *script_path;
    std::array<roi, 52> rois;
};
struct _websocket
{
    bool enabled;
    bool ws_secured;
    bool http_secured;
    int port;
    int loglevel;
    int first_image_delay;
    const char *name;
    const char *usertoken{""};
};

class CFG
{
public:
    bool config_loaded = false;
    libconfig::Config lc{};
    std::string filePath{};

    CFG();
    void load();
    static CFG *createNew();
    bool readConfig();
    bool updateConfig();

#if defined(AUDIO_SUPPORT)
    _audio audio{};
#endif
    _general general{};
    _rtsp rtsp{};
    _sensor sensor{};
    _image image{};
    _stream stream0{};
    _stream stream1{};
    _stream stream2{};
    _motion motion{};
    _websocket websocket{};

    int numOsdConfigItems{0};
    OsdConfigItem *osdConfigItems;
    
    void deleteOsdConfigItem(int indexToDelete)
    {
        if (indexToDelete < 0 || indexToDelete >= numOsdConfigItems)
            return;

        int n = 0;

        OsdConfigItem* newItems = new OsdConfigItem[numOsdConfigItems - 1];

        for (int i = 0; i < numOsdConfigItems; ++i) {
            if (i!=indexToDelete)
                newItems[n++].assign_or_update(&osdConfigItems[i]);
        }

        std::unique_lock lck(mutex_main);
        delete[] osdConfigItems;
        osdConfigItems = newItems;
        numOsdConfigItems = n;
    }

    void addOsdConfigItem(OsdConfigItem *newItem) {
        OsdConfigItem* newItems = new OsdConfigItem[numOsdConfigItems + 1];

        for (int i = 0; i < numOsdConfigItems; ++i) {
            newItems[i].assign_or_update(&osdConfigItems[i]);
        }

        newItems[numOsdConfigItems].assign_or_update(newItem);
        
        std::unique_lock lck(mutex_main);
        delete[] osdConfigItems;
        osdConfigItems = newItems;
        numOsdConfigItems++;
    }

    template <typename T>
    T get(const std::string &name)
    {
        T result = T{};
        std::vector<ConfigItem<T>> *items = nullptr;
        if constexpr (std::is_same_v<T, bool>)
        {
            items = &boolItems;
        }
        else if constexpr (std::is_same_v<T, const char *>)
        {
            items = &charItems;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            items = &intItems;
        }
        else if constexpr (std::is_same_v<T, unsigned int>)
        {
            items = &uintItems;
        }
        else
        {
            return result;
        }
        for (auto &item : *items)
        {
            if (item.path == name)
            {
                return item.value;
            }
        }
        return result;
    }

    template <typename T>
    bool set(const std::string &name, T value, bool noSave = false)
    {
        std::vector<ConfigItem<T>> *items = nullptr;
        if constexpr (std::is_same_v<T, bool>)
        {
            items = &boolItems;
        }
        else if constexpr (std::is_same_v<T, const char *>)
        {
            items = &charItems;
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            items = &intItems;
        }
        else if constexpr (std::is_same_v<T, unsigned int>)
        {
            items = &uintItems;
        }
        else
        {
            return false;
        }
        for (auto &item : *items)
        {
            if (item.path == name)
            {
                if (item.validate(value))
                {
                    item.value = value;
                    item.noSave = noSave;
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
        return false;
    }

private:
    std::vector<ConfigItem<bool>> boolItems{};
    std::vector<ConfigItem<const char *>> charItems{};
    std::vector<ConfigItem<int>> intItems{};
    std::vector<ConfigItem<unsigned int>> uintItems{};

    std::vector<ConfigItem<bool>> getBoolItems();
    std::vector<ConfigItem<const char *>> getCharItems();
    std::vector<ConfigItem<int>> getIntItems();
    std::vector<ConfigItem<unsigned int>> getUintItems();
};

// The configuration is kept in a global singleton that's accessed via this
// shared_ptr.
extern std::shared_ptr<CFG> cfg;
