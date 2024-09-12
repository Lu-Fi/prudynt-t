#include <cmath>
#include "OSD.hpp"


#if defined(PLATFORM_T31)
#define IMPEncoderCHNAttr IMPEncoderChnAttr
#define IMPEncoderCHNStat IMPEncoderChnStat
#endif

#if defined(PLATFORM_T31)
#define picWidth uWidth
#define picHeight uHeight
#endif

#include <vector>

#include "schrift.h"

int OSD::renderGlyph(const char *characters)
{

    while (*characters)
    {

        SFT_LMetrics lmetrics;
        SFT_GMetrics gmetrics;
        SFT_Glyph glyph;
        SFT_Image imageBuffer;

        if (sft_lmetrics(sft, &lmetrics) == 0 && sft_lookup(sft, *characters, &glyph) == 0)
        {
            if (sft_gmetrics(sft, glyph, &gmetrics) == 0)
            {
                imageBuffer.width = gmetrics.minWidth;
                imageBuffer.height = gmetrics.minHeight;
                imageBuffer.pixels = (uint8_t *)malloc(imageBuffer.width * imageBuffer.height);

                if (sft_render(sft, glyph, imageBuffer) == 0)
                {
                    Glyph g;
                    g.width = imageBuffer.width;
                    g.height = imageBuffer.height;
                    g.advance = gmetrics.advanceWidth;
                    g.xmin = gmetrics.leftSideBearing;
                    g.ymin = gmetrics.yOffset;
                    g.glyph = glyph;

                    g.bitmap.resize(g.width * g.height * 4);
                    for (int y = 0; y < g.height; ++y)
                    {
                        for (int x = 0; x < g.width; ++x)
                        {
                            int pixelIndex = y * g.width + x;
                            uint8_t alpha = ((uint8_t *)imageBuffer.pixels)[pixelIndex];
                            if (alpha > 0)
                            {
                                g.bitmap[pixelIndex * 4] = BGRA_TEXT[0];
                                g.bitmap[pixelIndex * 4 + 1] = BGRA_TEXT[1];
                                g.bitmap[pixelIndex * 4 + 2] = BGRA_TEXT[2];
                                g.bitmap[pixelIndex * 4 + 3] = alpha;
                            }
                        }
                    }

                    glyphs[*characters] = g;
                }
                free(imageBuffer.pixels);
            }
        }
        ++characters;
    }

    return 0;
}

void setPixel(uint8_t *image, int x, int y, const uint8_t *color, int WIDTH, int HEIGHT)
{
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
    {
        int index = (y * WIDTH + x) * 4;
        image[index] = color[0];     // B
        image[index + 1] = color[1]; // G
        image[index + 2] = color[2]; // R
        image[index + 3] = color[3]; // A
    }
}

void OSD::drawOutline(uint8_t *image, const Glyph &g, int x, int y, int outlineSize, int WIDTH, int HEIGHT)
{
    for (int j = -outlineSize; j <= outlineSize; ++j)
    {
        for (int i = -outlineSize; i <= outlineSize; ++i)
        {
            if (i * i + j * j <= outlineSize * outlineSize)
            { // Use circular distance
                for (int h = 0; h < g.height; ++h)
                {
                    for (int w = 0; w < g.width; ++w)
                    {
                        int srcIndex = (h * g.width + w) * 4;
                        if (g.bitmap[srcIndex + 3] > 0)
                        { // Check alpha value
                            setPixel(image, x + w + i, y + h + j, BGRA_STROKE, WIDTH, HEIGHT);
                        }
                    }
                }
            }
        }
    }
}
/*
void OSD::drawOutline(uint8_t *image, const Glyph &g, int x, int y, int outlineSize, int WIDTH, int HEIGHT)
{
    for (int j = -outlineSize; j <= outlineSize; ++j)
    {
        for (int i = -outlineSize; i <= outlineSize; ++i)
        {
            if (abs(i) + abs(j) <= outlineSize)
            { // Use Manhattan distance
                for (int h = 0; h < g.height; ++h)
                {
                    for (int w = 0; w < g.width; ++w)
                    {
                        int srcIndex = (h * g.width + w) * 4;
                        if (g.bitmap[srcIndex + 3] > 0)
                        { // Check alpha value
                            setPixel(image, x + w + i, y + h + j, BGRA_STROKE, WIDTH, HEIGHT);
                        }
                    }
                }
            }
        }
    }
}
*/
int OSD::drawText(uint8_t *image, const char *text, int WIDTH, int HEIGHT, int outlineSize)
{
    int penX = 1;
    int penY = 1;

    // Draw text and outline
    while (*text)
    {
        auto it = glyphs.find(*text);
        if (it != glyphs.end())
        {
            const Glyph &g = it->second;

            int x = penX + g.xmin + outlineSize;
            int y = penY + (sft->yScale + g.ymin);

            // Draw the outline
            drawOutline(image, g, x, y, outlineSize, WIDTH, HEIGHT);

            // Draw the actual text
            for (int j = 0; j < g.height; ++j)
            {
                for (int i = 0; i < g.width; ++i)
                {
                    int srcIndex = (j * g.width + i) * 4;
                    if (g.bitmap[srcIndex + 3] > 0)
                    { // Check alpha value
                        setPixel(image, x + i, y + j, &g.bitmap[srcIndex], WIDTH, HEIGHT);
                    }
                }
            }

            penX += g.advance + (outlineSize * 2);
        }
        ++text;
    }

    return 0;
}

int OSD::calculateTextSize(const char *text, uint16_t &width, uint16_t &height, int outlineSize)
{
    width = 0;
    height = 0;

    while (*text)
    {
        auto it = glyphs.find(*text);
        if (it != glyphs.end())
        {
            const Glyph &g = it->second;

            width += g.advance + (outlineSize * 2);
            if (g.height > height)
            {
                height = g.height;
            }
        }

        ++text;
    }

    height += sft->yScale;
    width += 1 + outlineSize;

    return 0;
}

int OSD::libschrift_init()
{
    LOG_DEBUG("OSD::libschrift_init()");

    FILE *fontFile = fopen(osd.font_path, "rb");
    if (!fontFile)
    {
        LOG_DEBUG("Unable to open font file.");
        return -1;
    }

    BGRA_TEXT[2] = (osd.font_color >> 16) & 0xFF; // Blue
    BGRA_TEXT[1] = (osd.font_color >> 8) & 0xFF;  // Green
    BGRA_TEXT[0] = (osd.font_color >> 0) & 0xFF;  // Red
    BGRA_TEXT[3] = 0;                             // Alpha

    BGRA_STROKE[2] = (osd.font_stroke_color >> 16) & 0xFF; // Blue
    BGRA_STROKE[1] = (osd.font_stroke_color >> 8) & 0xFF;  // Green
    BGRA_STROKE[0] = (osd.font_stroke_color >> 0) & 0xFF;  // Red
    BGRA_STROKE[3] = 255;                                  // Alpha

    fseek(fontFile, 0, SEEK_END);
    long fileSize = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);

    uint8_t *fontData = (uint8_t *)malloc(fileSize);
    if (!fontData)
    {
        fclose(fontFile);
        LOG_DEBUG("Memory allocation failed for font data.");
        return -1;
    }

    size_t bytesRead = fread(fontData, 1, fileSize, fontFile);
    fclose(fontFile);
    LOG_DEBUG(bytesRead);
    
    if (bytesRead != (size_t)fileSize)
    {
        free(fontData);
        LOG_DEBUG("Error reading font file.");
        return -1;
    }

    sft = new SFT();
    sft->flags = SFT_DOWNWARD_Y;
    sft->xScale = osd.font_size * osd.font_xscale / 100;
    sft->yScale = osd.font_size * osd.font_yscale / 100;
    sft->yOffset = osd.font_yoffset;

    sft->font = sft_loadmem(fontData, fileSize);

    if (!sft->font)
    {
        delete sft; 
        free(fontData);
        LOG_DEBUG("Unable to load font data.");
        return -1;
    }

    LOG_DDEBUG("prerender OSD glyph's, xScale:" << sft->xScale << " yScale:" << sft->yScale);
    renderGlyph("01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!§$%&/()=?,.-_:;#'+*~}{} ");

    free(fontData);
    sft->~SFT();

    LOG_DDEBUG("OSD::libschrift_init() done.");
    return 0;
}

void OSD::set_text(OSDItem *osdItem, IMPOSDRgnAttr *rgnAttr, const char *text, int posX, int posY, int angle)
{

    // size and stroke
    uint8_t stroke_width = osd.font_stroke;
    uint16_t item_width = 0;
    uint16_t item_height = 0;

    calculateTextSize(text, item_width, item_height, stroke_width);

    if (item_width % 2 != 0)
        ++item_width;

    int item_size = item_width * item_height * 4;

    free(osdItem->data);
    osdItem->data = (uint8_t *)malloc(item_size);
    memset(osdItem->data, 0, item_size);

    drawText(osdItem->data, text, item_width, item_height, stroke_width);

    if (angle)
    {
        rotateBGRAImage(osdItem->data, item_width, item_height, angle, true);
    }

    if (item_width != osdItem->width || item_height != osdItem->height)
    {
        if (rgnAttr == nullptr)
        {
            rgnAttr = new IMPOSDRgnAttr();
            IMP_OSD_GetRgnAttr(osdItem->imp_rgn, rgnAttr);
        }

        set_pos(rgnAttr, posX, posY, item_width, item_height, stream_width, stream_height);

        rgnAttr->data.picData.pData = osdItem->data;
        osdItem->rgnAttrData = &rgnAttr->data;

        osdItem->width = item_width;
        osdItem->height = item_height;

        IMP_OSD_SetRgnAttr(osdItem->imp_rgn, rgnAttr);
    }
    else
    {
        osdItem->rgnAttrData->picData.pData = osdItem->data;
        IMP_OSD_UpdateRgnAttrData(osdItem->imp_rgn, osdItem->rgnAttrData);
    }

    return;
}

void OSD::set_text2(OSDItemV2 *osdItem, IMPOSDRgnAttr *rgnAttr, const char *text, int posX, int posY, int angle)
{

    // size and stroke
    uint8_t stroke_width = osd.font_stroke;
    uint16_t item_width = 0;
    uint16_t item_height = 0;

    calculateTextSize(text, item_width, item_height, stroke_width);

    if (item_width % 2 != 0)
        ++item_width;

    int item_size = item_width * item_height * 4;

    free(osdItem->data);
    osdItem->data = (uint8_t *)malloc(item_size);
    memset(osdItem->data, 0, item_size);

    drawText(osdItem->data, text, item_width, item_height, stroke_width);

    if (angle)
    {
        rotateBGRAImage(osdItem->data, item_width, item_height, angle, true);
    }

    if (item_width != osdItem->width || item_height != osdItem->height)
    {
        if (rgnAttr == nullptr)
        {
            rgnAttr = new IMPOSDRgnAttr();
            IMP_OSD_GetRgnAttr(osdItem->imp_rgn, rgnAttr);
        }

        set_pos(rgnAttr, posX, posY, item_width, item_height, stream_width, stream_height);

        rgnAttr->data.picData.pData = osdItem->data;
        osdItem->rgnAttrData = &rgnAttr->data;

        osdItem->width = item_width;
        osdItem->height = item_height;

        IMP_OSD_SetRgnAttr(osdItem->imp_rgn, rgnAttr);
    }
    else
    {
        osdItem->rgnAttrData->picData.pData = osdItem->data;
        IMP_OSD_UpdateRgnAttrData(osdItem->imp_rgn, osdItem->rgnAttrData);
    }

    return;
}

unsigned long getSystemUptime()
{
    struct sysinfo info;
    if (sysinfo(&info) != 0)
    {
        return 0;
    }
    return info.uptime;
}

int getIp(char *addressBuffer)
{
    struct ifaddrs *ifAddrStruct = nullptr;
    struct ifaddrs *ifa = nullptr;
    void *tmpAddrPtr = nullptr;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
        {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET)
        { // check it is IP4
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        }
    }
    if (ifAddrStruct != nullptr)
        freeifaddrs(ifAddrStruct);
    return 0;
}

int autoFontSize(int pWidth)
{
    double m = 0.0046875;
    double b = 9.0;
    return static_cast<int>(m * pWidth + b + 0.5);
}

void replacestr(char *line, const char *search, const char *replace)
{
   int count;
   char *sp;

   if ((sp = strstr(line, search)) == NULL) {
      return;
   }
   int sLen = strlen(search);
   int rLen = strlen(replace);
   if (sLen > rLen) {
      char *src = sp + sLen;
      char *dst = sp + rLen;
      while((*dst = *src) != '\0') { dst++; src++; }
   } else if (sLen < rLen) {
      int tLen = strlen(sp) - sLen;
      char *stop = sp + rLen;
      char *src = sp + sLen + tLen;
      char *dst = sp + rLen + tLen;
      while(dst >= stop) { *dst = *src; dst--; src--; }
   }
   memcpy(sp, replace, rLen);
   replacestr(sp + rLen, search, replace);
}

void OSD::rotateBGRAImage(uint8_t *&inputImage, uint16_t &width, uint16_t &height, int angle, bool del = true)
{
    double angleRad = angle * (M_PI / 180.0);

    int originalCorners[4][2] = {
        {0, 0},
        {width, 0},
        {0, height},
        {width, height}};

    int minX = INT_MAX;
    int maxX = INT_MIN;
    int minY = INT_MAX;
    int maxY = INT_MIN;

    for (auto &originalCorner : originalCorners)
    {
        int x = originalCorner[0];
        int y = originalCorner[1];

        int newX = static_cast<int>(x * cos(angleRad) - y * sin(angleRad));
        int newY = static_cast<int>(x * sin(angleRad) + y * cos(angleRad));

        if (newX < minX)
            minX = newX;
        if (newX > maxX)
            maxX = newX;
        if (newY < minY)
            minY = newY;
        if (newY > maxY)
            maxY = newY;
    }

    int newWidth = maxX - minX + 1;
    int newHeight = maxY - minY + 1;

    int centerX = width / 2;
    int centerY = height / 2;

    int newCenterX = newWidth / 2;
    int newCenterY = newHeight / 2;

    auto *rotatedImage = new uint8_t[newWidth * newHeight * 4]();

    for (int y = 0; y < newHeight; ++y)
    {
        for (int x = 0; x < newWidth; ++x)
        {
            int newX = x - newCenterX;
            int newY = y - newCenterY;

            int origX = static_cast<int>(newX * cos(angleRad) + newY * sin(angleRad)) + centerX;
            int origY = static_cast<int>(-newX * sin(angleRad) + newY * cos(angleRad)) + centerY;

            if (origX >= 0 && origX < width && origY >= 0 && origY < height)
            {
                for (int c = 0; c < 4; ++c)
                {
                    rotatedImage[(y * newWidth + x) * 4 + c] = inputImage[(origY * width + origX) * 4 + c];
                }
            }
        }
    }

    if (del)
        delete[] inputImage;
    inputImage = rotatedImage;
    width = newWidth;
    height = newHeight;
}

uint16_t OSD::get_abs_pos(const uint16_t max, const uint16_t size, const int pos)
{
    if (pos == 0)
    {
        return max / 2 - size / 2;
    }
    if (pos < 0)
    {
        return max - size - 1 + pos;
    }
    return pos;
}

void OSD::set_pos(IMPOSDRgnAttr *rgnAttr, int x, int y, uint16_t width, uint16_t height, const uint16_t max_width, const uint16_t max_height)
{
    if (width == 0 || height == 0)
    {
        width = rgnAttr->rect.p1.x - rgnAttr->rect.p0.x + 1;
        height = rgnAttr->rect.p1.y - rgnAttr->rect.p0.y + 1;
    }

    if (x > max_width - width)
        x = max_width - width;

    if (y > max_height - height)
        y = max_height - height;

    rgnAttr->rect.p0.x = get_abs_pos(max_width, width, x);
    rgnAttr->rect.p0.y = get_abs_pos(max_height, height, y);
    rgnAttr->rect.p1.x = rgnAttr->rect.p0.x + width - 1;
    rgnAttr->rect.p1.y = rgnAttr->rect.p0.y + height - 1;
}

unsigned char *loadBGRAImage(const char *filepath, size_t &length)
{
    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        printf("Failed to open the OSD logo file.\n");
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *data = (unsigned char *)malloc(length);
    if (!data)
    {
        printf("Failed to allocate memory for the image.\n");
        fclose(file);
        return nullptr;
    }

    if (fread(data, 1, length, file) != length)
    {
        printf("Failed to read OSD logo data.\n");
        free(data);
        fclose(file);
        return nullptr;
    }

    fclose(file);
    return data;
}

bool OSD::OSDTextFromFile(OSDItemV2 *osdItem) 
{
    struct stat fileStat;
    LOG_DEBUG(osdItem->osdConfigItem->file);

    if (stat(osdItem->osdConfigItem->file, &fileStat) != 0) 
    {
        LOG_ERROR("unable to get stat: " << osdItem->osdConfigItem->file);
        return false;
    }

    if (fileStat.st_mtime != osdItem->file_time) 
    {
        osdItem->file_time = fileStat.st_mtime;

        char new_text[1024] = "-empty file-";
        memset(new_text, 0, sizeof(new_text));

        FILE *file = fopen(osdItem->osdConfigItem->file, "r");
        if (file != NULL) 
        {
            if (fgets(new_text, sizeof(new_text), file) == NULL) 
            {
                strncpy(new_text, "-empty file-", 12);
            }
            fclose(file);
        }
        else
        {
            strncpy(new_text, "-file error-", 12);
        }

        if (strcmp(new_text, osdItem->text) != 0)
        {
            osdItem->text = strdup(new_text);
            return true;
        }
    }

    return false;
}


bool OSD::OSDTextPlaceholders(OSDItemV2 *osdItem)
{
    char new_text[1024];
    strncpy(new_text, osdItem->osdConfigItem->text, sizeof(new_text) - 1);
    new_text[sizeof(new_text) - 1] = '\0';

    if (strstr(osdItem->osdConfigItem->text, "%hostname") != nullptr)
    {
        gethostname(hostname, 64);
        replacestr(new_text, "%hostname", hostname);
    }

    if (strstr(osdItem->osdConfigItem->text, "%ipaddress") != nullptr)
    {
        getIp(ip);
        replacestr(new_text, "%ipaddress", ip);
    }

    if (strstr(osdItem->osdConfigItem->text, "%fps") != nullptr)
    {
        char fps[4];
        snprintf(fps, 4, "%3d", osd.stats.fps);
        replacestr(new_text, "%fps", fps);
    }

    if (strstr(osdItem->osdConfigItem->text, "%bps") != nullptr)
    {
        char bps[8];
        snprintf(bps, 8, "%5d", osd.stats.bps);
        replacestr(new_text, "%bps", bps);
    }

    if (strstr(osdItem->osdConfigItem->text, "%uptime") != nullptr)
    {
        unsigned long currentUptime = getSystemUptime();
        unsigned long days = currentUptime / 86400;
        unsigned long hours = (currentUptime % 86400) / 3600;
        unsigned long minutes = (currentUptime % 3600) / 60;
        // unsigned long seconds = currentUptime % 60;
        snprintf(uptimeFormatted, sizeof(uptimeFormatted), osd.uptime_format, days, hours, minutes);
        replacestr(new_text, "%uptime", uptimeFormatted);
    }

    if (strstr(osdItem->osdConfigItem->text, "%datetime") != nullptr)
    {
        strftime(timeFormatted, sizeof(timeFormatted), osd.time_format, ltime);
        replacestr(new_text, "%datetime", timeFormatted);
    }

    if (strcmp(new_text, osdItem->text) != 0)
    {
        osdItem->text = strdup(new_text);
        return true;
    }

    return false;
}

OSD *OSD::createNew(
    _osd &osd,
    int osdGrp,
    int encChn,
    const char *parent)
{
    return new OSD(osd, osdGrp, encChn, parent);
}

void OSD::init()
{
    int ret = 0;
    LOG_DEBUG("OSD init encChn: " << encChn);

    // initially get time
    current = time(nullptr);
    ltime = localtime(&current);

    ret = IMP_OSD_SetPoolSize(cfg->general.osd_pool_size * 1024);
    LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_SetPoolSize(" << (cfg->general.osd_pool_size * 1024) << ")");

    last_updated_second = -1;

    ret = IMP_Encoder_GetChnAttr(osdGrp, &channelAttributes);
    if (ret < 0)
    {
        LOG_DEBUG("IMP_Encoder_GetChnAttr() == " << ret);
    }

    stream_width = channelAttributes.encAttr.picWidth;
    stream_height = channelAttributes.encAttr.picHeight;

    // picWidth, picHeight cpp macro !!
    LOG_DEBUG("IMP_Encoder_GetChnAttr read. Stream resolution: " << stream_width
                                                                 << "x" << stream_height);

    ret = IMP_OSD_CreateGroup(osdGrp);

    int fontSize = autoFontSize(channelAttributes.encAttr.picWidth);
    int autoOffset = round((float)(channelAttributes.encAttr.picWidth * 0.004));

    if (osd.font_size == OSD_AUTO_VALUE)
    {
        // use cfg->set to set noSave, so auto values will not written to config
        char tmpPath[22];
        snprintf(tmpPath, sizeof(tmpPath), "stream%d.osd.font_size\0", encChn);
        cfg->set<int>(tmpPath, fontSize, true);
    }

    if (libschrift_init() != 0)
    {
        LOG_DEBUG("libschrift init failed.");
    }

    //for (OsdConfigItem osdConfigItem : cfg->osdConfigItems)
    for (int i = 0; i < cfg->numOsdConfigItems; i++)
    {
        OsdConfigItem *osdConfigItem = &cfg->osdConfigItems[i];

        // check if this osdConfigItem is assigned to this encChn(stream)
        if (osdConfigItem->streams[encChn])
        {
            OSDItemV2 *osdItem = new OSDItemV2(osdConfigItem, osdGrp);

            LOG_DEBUG("osdConfigItem " << i << " {" << 
                "'posX':" << osdItem->osdConfigItem->posX << ", " <<
                "'posY':" << osdItem->osdConfigItem->posY << ", " <<
                "'height':" << osdItem->osdConfigItem->height << ", " <<
                "'width':" << osdItem->osdConfigItem->width << ", " <<
                "'text':'" << (osdItem->osdConfigItem->text ? osdItem->osdConfigItem->text : "") << "', " <<
                "'file':'" << (osdItem->osdConfigItem->file ? osdItem->osdConfigItem->file : "") << "'}"
            );

            // OSD Image
            if (osdItem->osdConfigItem->width && osdItem->osdConfigItem->height)
            {
                LOG_DEBUG("OSDImage");
                IMPOSDRgnAttr rgnAttr;
                memset(&rgnAttr, 0, sizeof(IMPOSDRgnAttr));
                IMP_OSD_GetRgnAttr(osdItem->imp_rgn, &rgnAttr);

                size_t imageSize;
                osdItem->data = loadBGRAImage(osdItem->osdConfigItem->file, imageSize);

                // Verify OSD logo size vs dimensions
                if ((osd.logo_width * osd.logo_height * 4) == imageSize)
                {
                    rgnAttr.data.picData.pData = osdItem->data;

                    // Logo rotation
                    uint16_t logo_width = osdItem->osdConfigItem->width;
                    uint16_t logo_height = osdItem->osdConfigItem->height;
                    if (osdItem->osdConfigItem->rotation)
                    {
                        uint8_t *imageData = static_cast<uint8_t *>(rgnAttr.data.picData.pData);
                        rotateBGRAImage(imageData, logo_width,
                                        logo_height, osdItem->osdConfigItem->rotation, false);
                        rgnAttr.data.picData.pData = imageData;
                    }

                    set_pos(&rgnAttr, osdItem->osdConfigItem->posX,
                            osdItem->osdConfigItem->posY, logo_width, logo_height, stream_width, stream_height);
                }
                else
                {

                    LOG_ERROR("Invalid OSD logo dimensions. Imagesize=" << imageSize << ", " << osdItem->osdConfigItem->width
                                                                        << "*" << osdItem->osdConfigItem->height << "*4=" << (osdItem->osdConfigItem->width * osdItem->osdConfigItem->height * 4));
                }
                IMP_OSD_SetRgnAttr(osdItem->imp_rgn, &rgnAttr);
            }
            // OSD Text
            else if (osdItem->osdConfigItem->file)
            {
                LOG_DEBUG("OSDFile");
                if (OSDTextFromFile(osdItem))
                {
                    set_text2(osdItem, nullptr, osdItem->text,
                                osdItem->osdConfigItem->posX, osdItem->osdConfigItem->posY, osdItem->osdConfigItem->rotation);
                }
            }                
            // OSD Text
            else if (osdItem->osdConfigItem->text)
            {
                LOG_DEBUG("OSDText");
                if (OSDTextPlaceholders(osdItem))
                {
                    set_text2(osdItem, nullptr, osdItem->text,
                                osdItem->osdConfigItem->posX, osdItem->osdConfigItem->posY, osdItem->osdConfigItem->rotation);
                }
            }

            LOG_DEBUG("osdItem " << i << " {" << 
                "'text':" << osdItem->text << ", " <<
                "'length':" << strlen(osdItem->text) << ", " <<
                "'imp_rgn':" << osdItem->imp_rgn << "'}"
            );

            if (osdItem->data)
            {
                osdItems.push_back(osdItem);
            }
            else
            {
                LOG_ERROR("invalid osd item");

                ret = IMP_OSD_UnRegisterRgn(osdItem->imp_rgn, osdGrp);
                LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_UnRegisterRgn(" << osdItem->imp_rgn << ", " << osdGrp << ")");

                IMP_OSD_DestroyRgn(osdItem->imp_rgn);
            }
        }
    }

    if (osd.start_delay)
        startup_delay = (int)(osd.start_delay * 1000) / THREAD_SLEEP;
}

int OSD::start()
{
    int ret;

    ret = IMP_OSD_Start(osdGrp);
    LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_Start(" << osdGrp << ")");

    ret = IMP_OSD_SetPoolSize(cfg->general.osd_pool_size * 1024);
    LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_SetPoolSize(" << (cfg->general.osd_pool_size * 1024) << ")");

    is_started = true;

    return ret;
}

int OSD::exit()
{
    int ret;

    ret = IMP_OSD_Stop(osdGrp);
    LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_Stop(" << osdGrp << ")");

    for (OSDItemV2 *osdItem : osdItems)
    {

        if (osdItem)
        {
            ret = IMP_OSD_ShowRgn(osdItem->imp_rgn, osdGrp, 0);
            LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_ShowRgn(" << osdItem->imp_rgn << ", " << osdGrp << ", 0)");

            ret = IMP_OSD_UnRegisterRgn(osdItem->imp_rgn, osdGrp);
            LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_UnRegisterRgn(" << osdItem->imp_rgn << ", " << osdGrp << ")");

            IMP_OSD_DestroyRgn(osdItem->imp_rgn);

            free(osdItem->data);
        }
    }

    ret = IMP_OSD_DestroyGroup(osdGrp);
    LOG_DEBUG_OR_ERROR(ret, "IMP_OSD_DestroyGroup(" << osdGrp << ")");

    sft_freefont(sft->font);
    return 0;
}

void OSD::updateDisplayEverySecond()
{
    current = time(nullptr);
    ltime = localtime(&current);

    // Check if we have moved to a new second
    if (ltime->tm_sec != last_updated_second)
    {

        osd_items_to_update = osdItems.size();
        last_updated_second = ltime->tm_sec;
    }
    else
    {
        if (osd_items_to_update)
        {
            OSDItemV2 *osdItem = osdItems[osd_items_to_update - 1];

            if (osdItem->osdConfigItem->width && osdItem->osdConfigItem->height)
            {
            }
            else if (osdItem->osdConfigItem->text)
            {
                if (OSDTextPlaceholders(osdItem))
                {
                    LOG_DEBUG("OSDTextPlaceholders updated: " << osdItem->osdConfigItem->text << " == " << osdItem->text);
                    set_text2(osdItem, nullptr, osdItem->text,
                              osdItem->osdConfigItem->posX, osdItem->osdConfigItem->posY, osdItem->osdConfigItem->rotation);
                }
            }
            else if (osdItem->osdConfigItem->file)
            {
                if (OSDTextFromFile(osdItem))
                {
                    LOG_DEBUG("OSDFile updated: " << osdItem->osdConfigItem->file << " == " << osdItem->text);
                    set_text2(osdItem, nullptr, osdItem->text,
                              osdItem->osdConfigItem->posX, osdItem->osdConfigItem->posY, osdItem->osdConfigItem->rotation);
                }
            }
            osd_items_to_update--;
        }
    }
}
