#ifndef IMPFramesource_hpp
#define IMPFramesource_hpp

#include "Logger.hpp"
#include <imp/imp_framesource.h>

class IMPFramesource
{
public:
    static IMPFramesource *createNew(_stream *stream, _sensor *sensor, int chnNr);

    IMPFramesource(_stream *stream, _sensor *sensor, int chnNr) : stream(stream), sensor(sensor), chnNr(chnNr)
    {
        init();
    }

    ~IMPFramesource()
    {
        destroy();
    };

    int init();
    int enable();
    int disable();
    int destroy();
    bool initialized() {
        return (status == 3);
    };

private:
    _stream *stream{};
    _sensor *sensor{};
    int chnNr;
    int status{0};
};

#endif