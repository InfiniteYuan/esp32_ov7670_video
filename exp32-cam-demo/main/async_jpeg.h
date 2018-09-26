#define JPG_CONTENT_TYPE "image/jpeg"
#define PART_BOUNDARY "123456789000000000000987654321"

typedef struct {
        camera_fb_t * fb;
        size_t index;
} camera_frame_t;

static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace; boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: " JPG_CONTENT_TYPE "\r\nContent-Length: %u\r\n\r\n";

class AsyncJpegResponse: public AsyncAbstractResponse {
    private:
        camera_fb_t * fb;
        size_t _index;
    public:
        AsyncJpegResponse(camera_fb_t * frame){
            _callback = nullptr;
            _code = 200;
            _contentLength = frame->len;
            _contentType = JPG_CONTENT_TYPE;
            _index = 0;
            fb = frame;
        }
        ~AsyncJpegResponse(){
            if(fb != nullptr){
                camera_fb_return(fb);
            }
        }
        bool _sourceValid() const { return fb != nullptr; }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override{
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            memcpy(buffer, fb->buf+index, maxLen);
            if((index+maxLen) == fb->len){
                camera_fb_return(fb);
                fb = nullptr;
            }
            return maxLen;
        }
};

class AsyncJpegStreamResponse: public AsyncAbstractResponse {
    private:
        camera_frame_t _frame;
        size_t _index;
    public:
        AsyncJpegStreamResponse(){
            _callback = nullptr;
            _code = 200;
            _contentLength = 0;
            _contentType = STREAM_CONTENT_TYPE;
            _sendContentLength = false;
            _chunked = true;
            _index = 0;
            memset(&_frame, 0, sizeof(camera_frame_t));
        }
        ~AsyncJpegStreamResponse(){
            if(_index && _frame.fb){
                camera_fb_return(_frame.fb);
                _frame.fb = NULL;
            }
        }
        bool _sourceValid() const {
            return true;
        }
        virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
            size_t ret = _content(buf, maxLen, _index);
            if(ret != RESPONSE_TRY_AGAIN){
                _index += ret;
            }
            return ret;
        }
        size_t _content(uint8_t *buffer, size_t maxLen, size_t index){
            camera_frame_t * fb = (camera_frame_t*)&_frame;
            if(!fb->fb || fb->index == fb->fb->len){
                if(index && fb->fb){
                    camera_fb_return(fb->fb);
                    fb->fb = NULL;
                }
                if(maxLen < (strlen(STREAM_BOUNDARY) + strlen(STREAM_PART) + 10)){
                    //log_w("Not enough space for headers");
                    return RESPONSE_TRY_AGAIN;
                }
                //get frame
                fb->index = 0;

                // esp_err_t err = camera_start();
                // if (err != ESP_OK) {
                //     log_e("Camera capture failed with error = %d", err);
                //     return 0;
                // }

                // err = camera_wait();
                // if (err != ESP_OK) {
                //     log_e("Camera capture failed with error = %d", err);
                //     return 0;
                // }

                fb->fb = camera_fb_get();
                if (fb->fb == NULL) {
                    log_e("Camera frame failed");
                    return 0;
                }

                uint32_t sig = *((uint32_t *)fb->fb->buf) & 0xFFFFFF;
                if(sig != 0xffd8ff){
                    log_e("BAD SIG: 0x%x", sig);
                    camera_fb_return(fb->fb);
                    fb->fb = NULL;
                    return RESPONSE_TRY_AGAIN;
                }

                //send boundary
                size_t blen = 0;
                if(index){
                    blen = strlen(STREAM_BOUNDARY);
                    memcpy(buffer, STREAM_BOUNDARY, blen);
                    buffer += blen;
                }
                //send header
                size_t hlen = sprintf((char *)buffer, STREAM_PART, fb->fb->len);
                buffer += hlen;
                //send frame
                hlen = maxLen - hlen - blen;
                if(hlen > fb->fb->len){
                    maxLen -= hlen - fb->fb->len;
                    hlen = fb->fb->len;
                }
                memcpy(buffer, fb->fb->buf, hlen);
                fb->index += hlen;
                return maxLen;
            }

            size_t available = fb->fb->len - fb->index;
            if(maxLen > available){
                maxLen = available;
            }
            memcpy(buffer, fb->fb->buf+fb->index, maxLen);
            fb->index += maxLen;

            return maxLen;
        }
};
