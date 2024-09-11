#ifndef OSD_hpp
#define OSD_hpp

//#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "Config.hpp"
#include <imp/imp_osd.h>
#include <imp/imp_encoder.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include "schrift.h"
#include <filesystem>
#include "Config.hpp"
#include "Logger.hpp"

namespace fs = std::filesystem;

#if defined(PLATFORM_T31)
#define IMPEncoderCHNAttr IMPEncoderChnAttr
#define IMPEncoderCHNStat IMPEncoderChnStat
#endif

struct OSDItemV2
{
    IMPRgnHandle imp_rgn;
    uint8_t *data;
    uint16_t width;
    uint16_t height;
    std::string text;
    fs::file_time_type file_time;
    IMPOSDRgnAttrData *rgnAttrData;
    OsdConfigItem *osdConfigItem;

    OSDItemV2(OsdConfigItem *osdCi, int osdGrp) 
        : data(nullptr), width(0), height(0), text(""), rgnAttrData(nullptr), osdConfigItem(osdCi) {

        int ret;

        //osdConfigItem = new OsdConfigItem(*osdCi);

        imp_rgn = IMP_OSD_CreateRgn(nullptr);
        
        ret = IMP_OSD_RegisterRgn(imp_rgn, osdGrp, nullptr);
        LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_RegisterRgn(" << (int)imp_rgn << ", " << osdGrp << ", nullptr) == " << ret);

        IMPOSDRgnAttr rgnAttr;
        memset(&rgnAttr, 0, sizeof(IMPOSDRgnAttr));
        rgnAttr.type = OSD_REG_PIC;
        rgnAttr.fmt = PIX_FMT_BGRA;
        IMP_OSD_SetRgnAttr(imp_rgn, &rgnAttr);

        IMPOSDGrpRgnAttr grpRgnAttr;
        memset(&grpRgnAttr, 0, sizeof(IMPOSDGrpRgnAttr));
        grpRgnAttr.show = 1;
        grpRgnAttr.layer = 1;
        grpRgnAttr.gAlphaEn = 1;
        grpRgnAttr.fgAlhpa = osdConfigItem->transparency; /*transparency*/
        IMP_OSD_SetGrpRgnAttr(imp_rgn, osdGrp, &grpRgnAttr);        
    };
};

struct OSDItem
{
    IMPRgnHandle imp_rgn;
    uint8_t *data;
    uint16_t width;
    uint16_t height;
    IMPOSDRgnAttrData *rgnAttrData;
};

struct Glyph {
    int width;
    int height;
    std::vector<uint8_t> bitmap;
    int advance;
    int xmin;
    int ymin;
    SFT_Glyph glyph;
};

class OSD
{
public:
    static OSD *createNew(_osd &osd, int osdGrp, int encChn, const char *parent);
    OSD(_osd &osd, int osdGrp, int encChn, const char *parent) : osd(osd), osdGrp(osdGrp), encChn(encChn), parent(parent)
    {
        init();
    }
    void init();
    int exit();
    int start();

    void updateDisplayEverySecond();

    void rotateBGRAImage(uint8_t *&inputImage, uint16_t &width, uint16_t &height, int angle, bool del);
    static void set_pos(IMPOSDRgnAttr *rgnAttr, int x, int y, uint16_t width, uint16_t height, const uint16_t max_width, const uint16_t max_height);
    static uint16_t get_abs_pos(const uint16_t max,const uint16_t size,const int pos);
    int startup_delay{0};
    bool is_started = false;
    
private:

    // libschrift
    //std::vector<uint8_t> fontData;
    std::unordered_map<char, Glyph> glyphs;
    SFT *sft;
    int load_font();
    int libschrift_init();
    int renderGlyph(const char* characters);
    void drawOutline(uint8_t* image, const Glyph& g, int x, int y, int outlineSize, int WIDTH, int HEIGHT);
    int calculateTextSize(const char* text, uint16_t& width, uint16_t& height, int outlineSize);
    int drawText(uint8_t* image, const char* text, int WIDTH, int HEIGHT, int outlineSize);
    bool OSDTextPlaceholders(OSDItemV2 *osdItem);
    bool OSDTextFromFile(OSDItemV2 *osdItem);

    uint8_t BGRA_STROKE[4];
    uint8_t BGRA_TEXT[4];

    _osd &osd;
    int last_updated_second;
    const char *parent;

    OSDItem osdTime{};
    OSDItem osdUser{};
    OSDItem osdUptm{};
    OSDItem osdLogo{};

    std::vector<OSDItemV2 *> osdItems;

    void set_text(OSDItem *osdItem, IMPOSDRgnAttr *rgnAttr, const char *text, int posX, int posY, int angle);
    void set_text2(OSDItemV2 *osdItem, IMPOSDRgnAttr *rgnAttr, const char *text, int posX, int posY, int angle);
    std::string getConfigPath(const char *itemName);

    IMPEncoderCHNAttr channelAttributes;

    bool initialized{0};
    int osdGrp{};
    int encChn{};

    char hostname[64];
    char ip[INET_ADDRSTRLEN]{};

    uint16_t stream_width;
    uint16_t stream_height;

    time_t current;
    struct tm *ltime;
    struct timeval tm;

    char timeFormatted[32];
    char uptimeFormatted[32];
    char fps[4];
    char bps[8];
    uint8_t flag{0};
    uint8_t osd_items_to_update;
};

#endif
