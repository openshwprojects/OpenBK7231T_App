#include "../obkdef.h"
#include "../obkhelper.h"
#include "../new_common.h"
//#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../logging/logging.h"

#include "drv_idisplay.h"

static uint8_t    idisplay_init = 0;
static obk_list_t idisplay_list;

/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */
obk_err_t obk_display_renderer_register(obk_display_renderer_t *renderer) {
	if (idisplay_init != 1) {
		obk_list_init(&idisplay_list);
		idisplay_init = 1;
	}
	/* search? if idisplay name allready exists */
	obk_list_t *node = NULL;
    obk_list_for_each(node, &(idisplay_list)) {
		obk_service_t *_dev = obk_list_entry(node, obk_service_t, list);
		if (strcmp(_dev->name, renderer->parent.name) == 0) {			
			return OBK_ERROR;
		}
	}
	obk_init_mutex(&renderer->lock);
		 
	obk_list_insert_after(&(idisplay_list), &(renderer->parent.list));		
	return OBK_EOK;
}
/**
 * @brief This function 
 *
 * @param 
 *
 * @return 
 */							   
obk_err_t obk_display_attach_gui(obk_gui_service_t *service,
					            const char         *service_name,
					            void               *user_data) {
	if (idisplay_init != 0) {
		/* try to find object */
		obk_list_t *node = NULL;
		obk_list_for_each(node, &(idisplay_list)) {
			obk_service_t *_dev = obk_list_entry(node, obk_service_t, list);
			if (strcmp(_dev->name, service_name) == 0) {
				/* render finded */
				service->renderer = (obk_display_renderer_t *)_dev;
				/* init render */						
				return OBK_EOK;
			}
		}		
	}		
	addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "IDISPLAY not found");
	return OBK_ERROR;
}