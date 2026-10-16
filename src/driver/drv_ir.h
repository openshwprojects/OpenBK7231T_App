#ifndef __DRV_IR_H__
#define __DRV_IR_H__
#define SEND_MAXBITS 128

#ifdef __cplusplus
extern "C" {
#endif

#include "../httpserver/new_http.h"

void DRV_IR_Init();
void DRV_IR_RunFrame();
void DRV_IR_Deinit();
void DRV_IR_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);

#ifdef __cplusplus
}


//Fwd declaration(s)
struct PulsePauseWidthProtocolConstants;
class myIRsend;

//#include "../libraries/Arduino-IRremote-mod/src/IRProtocol.h"
class SpoofIrSender {
    public:
        void enableIROut(uint_fast8_t);
        void mark(unsigned int);
        void space(unsigned int);
        void sendPulseDistanceWidthFromArray(uint_fast8_t, unsigned int,unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, uint32_t*, unsigned int, bool,bool, unsigned int, int_fast8_t);
        void sendPulseDistanceWidthFromArray(PulsePauseWidthProtocolConstants *aProtocolConstants, uint32_t *aDecodedRawDataArray,
        unsigned int aNumberOfBits, int_fast8_t aNumberOfRepeats);
		void SetParent(myIRsend *IRParent);
	private:
		myIRsend * Parent;

};

#endif
#endif

