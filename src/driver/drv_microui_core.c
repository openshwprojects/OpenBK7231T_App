/*
** Copyright (c) 2024 rxi
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../new_common.h"
#include "../logging/logging.h"

#include "drv_microui_core.h"

#define unused(x) ((void) (x))

#define expect(x) do {                                               \
	if (!(x)) {                                                      \
	    addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "Fatal error: %s:%d: assertion '%s' failed\n", \
        __FILE__, __LINE__, #x);                                     \
		while(1);                                                    \
	}                                                                \
  } while (0)

#define push(stk, val, type) do {                                                 \
	expect((stk).idx < (stk).size); \
	uint8_t *dst = (uint8_t *)&(stk).items[(stk).idx]; \
	const uint8_t *src = (const uint8_t *)&(val);\
	memmove(dst, src, sizeof(type)); \
    (stk).idx++; /* incremented after incase `val` uses this value */           \
  } while (0)

#define pop(stk) do {      \
    expect((stk).idx > 0); \
    (stk).idx--;           \
  } while (0)

static mu_Rect unclipped_rect = { 0, 0, 0x1000, 0x1000 };

static mu_Style default_style = {
  /* font | font_size | size | padding | spacing | indent */
  NULL, 1, { 68, 10 }, 5, 4, 24,
  /* title_height | scrollbar_size | thumb_size */
  24, 12, 8,
  {
    { 230, 230, 230, 255 }, /* MU_COLOR_TEXT */
    { 25,  25,  25,  255 }, /* MU_COLOR_BORDER */
    { 50,  50,  50,  255 }, /* MU_COLOR_WINDOWBG */
    { 25,  25,  25,  255 }, /* MU_COLOR_TITLEBG */
    { 240, 240, 240, 255 }, /* MU_COLOR_TITLETEXT */
    { 0,   0,   0,   0   }, /* MU_COLOR_PANELBG */
    { 75,  75,  75,  255 }, /* MU_COLOR_BUTTON */
    { 95,  95,  95,  255 }, /* MU_COLOR_BUTTONHOVER */
    { 115, 115, 115, 255 }, /* MU_COLOR_BUTTONFOCUS */
    { 30,  30,  30,  255 }, /* MU_COLOR_BASE */
    { 35,  35,  35,  255 }, /* MU_COLOR_BASEHOVER */
    { 40,  40,  40,  255 }, /* MU_COLOR_BASEFOCUS */
    { 43,  43,  43,  255 }, /* MU_COLOR_SCROLLBASE */
    { 30,  30,  30,  255 }  /* MU_COLOR_SCROLLTHUMB */
  }
};

void copy_pos(mu_Vec2 *dst, const mu_Vec2 *pos) {
	memmove((uint8_t *)dst,(const uint8_t *)pos,sizeof(mu_Vec2));
}

void copy_rect(mu_Rect *dst, const mu_Rect *rect) {
	memmove((uint8_t *)dst,(const uint8_t *)rect,sizeof(mu_Rect));
}

void copy_color(mu_Color* dst, const mu_Color *color) {
	memmove((uint8_t *)dst,(const uint8_t *)color,sizeof(mu_Color));	
}

static void expand_rect(mu_Rect *rect, int16_t n, mu_Rect *result) {
  result->x = rect->x - n;
  result->y = rect->y - n;
  result->w = rect->w + n * 2;
  result->h = rect->h + n * 2;
}


static void intersect_rects(mu_Rect *r1, mu_Rect *r2, mu_Rect *r) {
  int16_t x1 = mu_max(r1->x, r2->x);
  int16_t y1 = mu_max(r1->y, r2->y);
  int16_t x2 = mu_min(r1->x + r1->w, r2->x + r2->w);
  int16_t y2 = mu_min(r1->y + r1->h, r2->y + r2->h);
  if (x2 < x1) { x2 = x1; }
  if (y2 < y1) { y2 = y1; }
  r->x = x1;
  r->y = y1; 
  r->w = x2 - x1;
  r->h = y2 - y1;
}


static int rect_overlaps_vec2(mu_Rect *r, mu_Vec2 *p) {
  return p->x >= r->x && p->x < r->x + r->w && p->y >= r->y && p->y < r->y + r->h;
}


static void draw_frame(mu_Context *ctx, mu_Rect *rect, uint8_t colorid) {  
  if (!(colorid == MU_COLOR_SCROLLBASE  ||
      colorid == MU_COLOR_SCROLLTHUMB ||
      colorid == MU_COLOR_TITLEBG)) { 
	  /* draw border */
	  if (ctx->style->colors[MU_COLOR_BORDER].a) {
		mu_Rect expanded_rect;
		expand_rect(rect, 1, &expanded_rect);
		mu_draw_box(ctx, &expanded_rect, &ctx->style->colors[MU_COLOR_BORDER]);
	  }
  }
  mu_draw_rect(ctx, rect, &ctx->style->colors[colorid]);
}


int8_t mu_init(mu_Context *ctx) {
  memset(ctx, 0, sizeof(mu_Context));
  ctx->command_list.items     = os_malloc(MU_COMMANDLIST_SIZE);
  if (!ctx->command_list.items) {
	  return -1;
  }
  ctx->command_list.size      = MU_COMMANDLIST_SIZE;
  memset((uint8_t*)ctx->command_list.items, 0, ctx->command_list.size);
  ctx->root_list.items        = (mu_Container**)os_malloc(MU_ROOTLIST_SIZE*sizeof(mu_Container*));
  if (!ctx->root_list.items) {
	  os_free(ctx->command_list.items);
	  return -1;
  }
  ctx->root_list.size         = MU_ROOTLIST_SIZE;
  memset((uint8_t*)ctx->root_list.items, 0, ctx->root_list.size);
  ctx->container_stack.items  = (mu_Container**)os_malloc(MU_CONTAINERSTACK_SIZE*sizeof(mu_Container*));
  if (!ctx->container_stack.items) {
	  os_free(ctx->command_list.items);
	  os_free(ctx->root_list.items);	  
	  return -1;
  }  
  ctx->container_stack.size   = MU_CONTAINERSTACK_SIZE;
  memset((uint8_t*)ctx->container_stack.items, 0, ctx->container_stack.size);
  ctx->clip_stack.items       = (mu_Rect*)os_malloc(MU_CLIPSTACK_SIZE*sizeof(mu_Rect));
  if (!ctx->clip_stack.items) {
	  os_free(ctx->command_list.items);
	  os_free(ctx->root_list.items);  
	  os_free(ctx->container_stack.items);
	  return -1;
  }
  ctx->clip_stack.size        = MU_CLIPSTACK_SIZE;
  memset((uint8_t*)ctx->clip_stack.items, 0, ctx->clip_stack.size);
  ctx->id_stack.items         = (mu_Id*)os_malloc(MU_IDSTACK_SIZE*sizeof(mu_Id));
  if (!ctx->id_stack.items) {
	  os_free(ctx->command_list.items);
	  os_free(ctx->root_list.items);  
	  os_free(ctx->container_stack.items);
	  os_free(ctx->clip_stack.items);
	  return -1;
  }  
  ctx->id_stack.size          = MU_IDSTACK_SIZE;
  memset((uint8_t*)ctx->id_stack.items, 0, ctx->id_stack.size);
  ctx->layout_stack.items     = (mu_Layout*)os_malloc(MU_LAYOUTSTACK_SIZE*sizeof(mu_Layout));
  if (!ctx->layout_stack.items) {
	  os_free(ctx->command_list.items);
	  os_free(ctx->root_list.items);  
	  os_free(ctx->container_stack.items);
	  os_free(ctx->clip_stack.items);
	  os_free(ctx->id_stack.items);
	  return -1;
  }
  ctx->layout_stack.size      = MU_LAYOUTSTACK_SIZE;  
  memset((uint8_t*)ctx->layout_stack.items, 0, ctx->layout_stack.size);
  ctx->draw_frame = draw_frame;
  memmove((uint8_t*)&ctx->_style, (uint8_t*)&default_style, sizeof(mu_Style));
  ctx->style = &ctx->_style;
  return 0;
}

int8_t mu_deinit(mu_Context *ctx) {
	if (ctx) {
		if (ctx->command_list.items)    {os_free(ctx->command_list.items);   } ctx->command_list.items = NULL;
		if (ctx->root_list.items)       {os_free(ctx->root_list.items);      } ctx->root_list.items = NULL;
		if (ctx->container_stack.items) {os_free(ctx->container_stack.items);} ctx->container_stack.items = NULL;
		if (ctx->clip_stack.items)      {os_free(ctx->clip_stack.items);     } ctx->clip_stack.items = NULL;
		if (ctx->id_stack.items)        {os_free(ctx->id_stack.items);       } ctx->id_stack.items = NULL; 
		if (ctx->layout_stack.items)    {os_free(ctx->layout_stack.items);	 } ctx->layout_stack.items = NULL;
	}
	return 0;
}


void mu_begin(mu_Context *ctx) {
  expect(ctx->text_width && ctx->text_height);
  ctx->command_list.idx = 0;
  ctx->root_list.idx = 0;
  ctx->scroll_target = NULL;
  ctx->hover_root = ctx->next_hover_root;
  ctx->next_hover_root = NULL;
  ctx->mouse_delta.x = ctx->mouse_pos.x - ctx->last_mouse_pos.x;
  ctx->mouse_delta.y = ctx->mouse_pos.y - ctx->last_mouse_pos.y;
  ctx->frame++;
}


static int compare_zindex(const void *a, const void *b) {
  return (*(mu_Container**) a)->zindex - (*(mu_Container**) b)->zindex;
}


void mu_end(mu_Context *ctx) {
  int i, n;
  /* check stacks */
  expect(ctx->container_stack.idx == 0);
  expect(ctx->clip_stack.idx      == 0);
  expect(ctx->id_stack.idx        == 0);
  expect(ctx->layout_stack.idx    == 0);


  /* handle scroll input */
  if (ctx->scroll_target) {
    ctx->scroll_target->scroll.x += ctx->scroll_delta.x;
    ctx->scroll_target->scroll.y += ctx->scroll_delta.y;
  }

  /* unset focus if focus id was not touched this frame */
  if (!ctx->updated_focus) { ctx->focus = 0; }
  ctx->updated_focus = 0;

  /* bring hover root to front if mouse was pressed */
  if (ctx->mouse_pressed && ctx->next_hover_root &&
      ctx->next_hover_root->zindex < ctx->last_zindex &&
      ctx->next_hover_root->zindex >= 0
  ) {
    mu_bring_to_front(ctx, ctx->next_hover_root);
  }

  /* reset input state */
  ctx->key_pressed = 0;
  ctx->input_text[0] = '\0';
  ctx->mouse_pressed = 0;
  //ctx->scroll_delta = mu_vec2(0, 0);
  ctx->scroll_delta.x = 0;
  ctx->scroll_delta.y = 0;
  ctx->last_mouse_pos = ctx->mouse_pos;

  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI and here");
  //rtos_delay_milliseconds(500);
  /* sort root containers by zindex */
  
  n = ctx->root_list.idx;
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI qsort %d", n);
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_end[%d]", __LINE__);
  //rtos_delay_milliseconds(500);  
  if (n>1) 
	qsort(ctx->root_list.items, n, sizeof(mu_Container*), compare_zindex);
  //return;
  //rtos_delay_milliseconds(500);
  /* set root container jump commands */
  for (i = 0; i < n; i++) {
    mu_Container *cnt = ctx->root_list.items[i];
    /* if this is the first container then make the first command jump to it.
    ** otherwise set the previous container's tail to jump to this one */
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI set root container jump commands,i=%d,cnt=%d", i, cnt);
	//rtos_delay_milliseconds(500);
    if (i == 0) {
      mu_Command *cmd = (mu_Command*) ctx->command_list.items;	  
	  if (cmd != 0) {
		//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_end[%d]", __LINE__);
		//rtos_delay_milliseconds(500);
		void *dst = (void*) cnt->head + sizeof(mu_JumpCommand);
		memmove((uint8_t*)&cmd->jump.dst, (uint8_t*)&dst, sizeof(void*));
		//cmd->jump.dst = (void*) ((char*) cnt->head + sizeof(mu_JumpCommand));
	  }
    } else {
      mu_Container *prev = ctx->root_list.items[i - 1];
	  if (prev != 0)
		prev->tail->jump.dst = (void*) ((char*)cnt->head + sizeof(mu_JumpCommand));
    }
    /* make the last container's tail jump to the end of command list */
    if (i == n - 1) {
		if (cnt->tail != 0) {
			//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_end[%d]", __LINE__);
			//rtos_delay_milliseconds(500);	
			void *dst = ctx->command_list.items + ctx->command_list.idx;
			memmove((uint8_t*)&cnt->tail->jump.dst, (uint8_t*)&dst, sizeof(void*));
			//cnt->tail->jump.dst = (void*) (ctx->command_list.items + ctx->command_list.idx);
		}
    }
  }
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_end exit");
  //rtos_delay_milliseconds(500);
}


void mu_set_focus(mu_Context *ctx, mu_Id id) {
  ctx->focus = id;
  ctx->updated_focus = 1;
}


/* 32bit fnv-1a hash */
#define HASH_INITIAL 2166136261

static void hash(mu_Id *hash, const void *data, int size) {
  const unsigned char *p = data;
  while (size--) {
    *hash = (*hash ^ *p++) * 16777619;
  }
}


mu_Id mu_get_id(mu_Context *ctx, const void *data, uint16_t size) {
  uint16_t idx = ctx->id_stack.idx;
  mu_Id res = (idx > 0) ? ctx->id_stack.items[idx - 1] : HASH_INITIAL;
  hash(&res, data, size);
  ctx->last_id = res;
  return res;
}


void mu_push_id(mu_Context *ctx, const void *data, uint16_t size) {
  mu_Id id = mu_get_id(ctx, data, size);
  push(ctx->id_stack, id, mu_Id);
}


void mu_pop_id(mu_Context *ctx) {
  pop(ctx->id_stack);
}


void mu_push_clip_rect(mu_Context *ctx, mu_Rect *rect) {
  mu_Rect *last = mu_get_clip_rect(ctx);
  mu_Rect intersect;
  intersect_rects(rect, last, &intersect);
  if (ctx->clip_stack.idx < ctx->clip_stack.size) {
	copy_rect(&ctx->clip_stack.items[ctx->clip_stack.idx], (const mu_Rect *)&intersect);
	ctx->clip_stack.idx++;
  } else {
	  // TODO: error
	  addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_push_clip_rect error");
      //rtos_delay_milliseconds(500);
  }
}


void mu_pop_clip_rect(mu_Context *ctx) {
  pop(ctx->clip_stack);
}


mu_Rect *mu_get_clip_rect(mu_Context *ctx) {
  expect(ctx->clip_stack.idx > 0);
  return &ctx->clip_stack.items[ctx->clip_stack.idx - 1];
}


int8_t mu_check_clip(mu_Context *ctx, mu_Rect *r) {
  mu_Rect *cr = mu_get_clip_rect(ctx);
  if (r->x > cr->x + cr->w || r->x + r->w < cr->x ||
      r->y > cr->y + cr->h || r->y + r->h < cr->y   ) { return MU_CLIP_ALL; }
  if (r->x >= cr->x && r->x + r->w <= cr->x + cr->w &&
      r->y >= cr->y && r->y + r->h <= cr->y + cr->h ) { return 0; }
  return MU_CLIP_PART;
}


static void push_layout(mu_Context *ctx, mu_Rect *body, mu_Vec2 *scroll) {
  mu_Layout layout;
  int16_t width = 0;
  memset(&layout, 0, sizeof(layout));
  //layout.body = mu_rect(body.x - scroll.x, body.y - scroll.y, body.w, body.h);
  layout.body.x = body->x - scroll->x;
  layout.body.y = body->y - scroll->y;
  layout.body.w = body->w;
  layout.body.h = body->h;
  //layout.max = mu_vec2(-0x1000, -0x1000);
  layout.max.x = -0x1000;
  layout.max.y = -0x1000;
  if (ctx->layout_stack.idx < ctx->layout_stack.size) {
	memmove((uint8_t*)&ctx->layout_stack.items[ctx->layout_stack.idx], (uint8_t*)&layout, sizeof(mu_Layout));
	ctx->layout_stack.idx++;
  }
  
  mu_layout_row(ctx, 1, &width, 0);
}


static mu_Layout* get_layout(mu_Context *ctx) {
  return &ctx->layout_stack.items[ctx->layout_stack.idx - 1];
}


static void pop_container(mu_Context *ctx) {
  mu_Container *cnt = mu_get_current_container(ctx);
  mu_Layout *layout = get_layout(ctx);
  cnt->content_size.x = layout->max.x - layout->body.x;
  cnt->content_size.y = layout->max.y - layout->body.y;
  /* pop container, layout and id */
  pop(ctx->container_stack);
  pop(ctx->layout_stack);
  mu_pop_id(ctx);
}


mu_Container* mu_get_current_container(mu_Context *ctx) {
  expect(ctx->container_stack.idx > 0);
  return ctx->container_stack.items[ ctx->container_stack.idx - 1 ];
}


static mu_Container* get_container(mu_Context *ctx, mu_Id id, int opt) {
  mu_Container *cnt;
  /* try to get existing container from pool */
  int idx = mu_pool_get(ctx, ctx->container_pool, MU_CONTAINERPOOL_SIZE, id);
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI get_container idx=%d", idx);
  //rtos_delay_milliseconds(500);
  
  if (idx >= 0) {
    if (ctx->containers[idx].open || ~opt & MU_OPT_CLOSED) {
      mu_pool_update(ctx, ctx->container_pool, idx);
    }
    return &ctx->containers[idx];
  }
  if (opt & MU_OPT_CLOSED) { return NULL; }
  /* container not found in pool: init new container */
  idx = mu_pool_init(ctx, ctx->container_pool, MU_CONTAINERPOOL_SIZE, id);
  cnt = &ctx->containers[idx];
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_pool_init idx=%d", idx);
  memset(cnt, 0, sizeof(*cnt));
  cnt->open = 1;
  mu_bring_to_front(ctx, cnt);
  return cnt;
}


mu_Container* mu_get_container(mu_Context *ctx, const char *name) {
  mu_Id id = mu_get_id(ctx, name, strlen(name));
  return get_container(ctx, id, 0);
}


void mu_bring_to_front(mu_Context *ctx, mu_Container *cnt) {
  cnt->zindex = ++ctx->last_zindex;
}


/*============================================================================
** pool
**============================================================================*/

int8_t mu_pool_init(mu_Context *ctx, mu_PoolItem *items, uint8_t len, mu_Id id) {
  int i, n = -1, f = ctx->frame;
  for (i = 0; i < len; i++) {
    if (items[i].last_update < f) {
      f = items[i].last_update;
      n = i;
    }
  }
  expect(n > -1);
  items[n].id = id;
  mu_pool_update(ctx, items, n);
  return n;
}


int8_t mu_pool_get(mu_Context *ctx, mu_PoolItem *items, uint8_t len, mu_Id id) {
  int8_t i;
  unused(ctx);
  for (i = 0; i < len; i++) {
    if (items[i].id == id) { return i; }
  }
  return -1;
}


void mu_pool_update(mu_Context *ctx, mu_PoolItem *items, int8_t idx) {
  items[idx].last_update = ctx->frame;
}


/*============================================================================
** input handlers
**============================================================================*/

void mu_input_mousemove(mu_Context *ctx, int x, int y) {	
  ctx->mouse_pos.x = x;
  ctx->mouse_pos.y = y;
}


void mu_input_mousedown(mu_Context *ctx, int x, int y, int btn) {
  mu_input_mousemove(ctx, x, y);
  ctx->mouse_down |= btn;
  ctx->mouse_pressed |= btn;
}


void mu_input_mouseup(mu_Context *ctx, int x, int y, int btn) {
  mu_input_mousemove(ctx, x, y);
  ctx->mouse_down &= ~btn;
}


void mu_input_scroll(mu_Context *ctx, int x, int y) {
  ctx->scroll_delta.x += x;
  ctx->scroll_delta.y += y;
}


void mu_input_keydown(mu_Context *ctx, int key) {
  ctx->key_pressed |= key;
  ctx->key_down |= key;
}


void mu_input_keyup(mu_Context *ctx, int key) {
  ctx->key_down &= ~key;
}


void mu_input_text(mu_Context *ctx, const char *text) {
  int len = strlen(ctx->input_text);
  int size = strlen(text) + 1;
  expect(len + size <= (int) sizeof(ctx->input_text));
  memcpy(ctx->input_text + len, text, size);
}


/*============================================================================
** commandlist
**============================================================================*/

mu_Command* mu_push_command(mu_Context *ctx, uint8_t type, uint8_t size) { 
  mu_Command *cmd = (mu_Command*) (ctx->command_list.items + ctx->command_list.idx);
  expect(ctx->command_list.idx + size < MU_COMMANDLIST_SIZE);
  cmd->base.type = type;
  cmd->base.size = size;
  ctx->command_list.idx += size;
  return cmd;
}


int8_t mu_next_command(mu_Context *ctx, mu_Command **cmd) {
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_next_command 0, cmd=%d", *cmd);
  //rtos_delay_milliseconds(500);
  if (*cmd) {
    *cmd = (mu_Command*) (((char*) *cmd) + (*cmd)->base.size);
  } else {
    *cmd = (mu_Command*) ctx->command_list.items;
  }
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_next_command 1, cmd=%d", *cmd);
  //rtos_delay_milliseconds(500);
  while ((char*) *cmd != ctx->command_list.items + ctx->command_list.idx) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MicroUI mu_next_command");
    //rtos_delay_milliseconds(500);
	
    if ((*cmd)->type != MU_COMMAND_JUMP) { return 1; }
    //*cmd = (*cmd)->jump.dst;
	memmove((uint8_t*)(cmd), (uint8_t*)&(*cmd)->jump.dst, sizeof(mu_Command *));
  }
  return 0;
}


static mu_Command* push_jump(mu_Context *ctx, mu_Command *dst) {
  mu_Command *cmd;
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MU_COMMAND_JUMP: dst=%d", dst); 
  cmd = mu_push_command(ctx, MU_COMMAND_JUMP, sizeof(mu_JumpCommand));
  //cmd->jump.dst = dst;
  mu_Command *_dst = (mu_Command *)dst; 
  memmove((uint8_t*)&cmd->jump.dst, (uint8_t*)&_dst, sizeof(mu_Command *));
  return cmd;
}


void mu_set_clip(mu_Context *ctx, mu_Rect *rect) {
  mu_Command *cmd;
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MU_COMMAND_CLIP: rect=[%d,%d,%d,%d]", rect->x, rect->y, rect->w, rect->h); 
  cmd = mu_push_command(ctx, MU_COMMAND_CLIP, sizeof(mu_ClipCommand));
  //cmd->clip.rect = rect;
  copy_rect(&cmd->clip.rect, (const mu_Rect *)rect);
}


void mu_draw_rect(mu_Context *ctx, mu_Rect *rect, mu_Color *color) {
  mu_Command *cmd;
  mu_Rect intersect;
  intersect_rects(rect, mu_get_clip_rect(ctx), &intersect);
  if (intersect.w > 0 && intersect.h > 0) {
	//addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MU_COMMAND_RECT: rect=[%d,%d,%d,%d]", intersect.x, intersect.y, intersect.w, intersect.h);
    cmd = mu_push_command(ctx, MU_COMMAND_RECT, sizeof(mu_RectCommand));
    //cmd->rect.rect = rect;
	copy_rect( &cmd->rect.rect, (const mu_Rect *)&intersect );
    //cmd->rect.color = color;
	copy_color( &cmd->rect.color, (const mu_Color*)color );	
  }
}


void mu_draw_box(mu_Context *ctx, mu_Rect *rect, mu_Color *color) {
  mu_Rect r;
  r.x = rect->x + 1; r.y = rect->y; r.w = rect->w - 2; r.h = 1;
  mu_draw_rect(ctx, &r, color);
  r.x=rect->x + 1; r.y=rect->y + rect->h - 1; r.w=rect->w - 2; r.h=1;
  mu_draw_rect(ctx, &r, color);
  r.x=rect->x; r.y=rect->y; r.w=1; r.h=rect->h;
  mu_draw_rect(ctx, &r, color);
  r.x=rect->x + rect->w - 1; r.y=rect->y; r.w=1; r.h=rect->h;
  mu_draw_rect(ctx, &r, color);
}


void mu_draw_text(mu_Context *ctx, mu_Font *font, const char *str, int8_t len, mu_Vec2 *pos, mu_Color *color, uint8_t size)
{
  mu_Command *cmd;
  mu_Rect rect;
  rect.x = pos->x;
  rect.y = pos->y;
  rect.w = ctx->text_width(font, size, str, len);
  rect.h = ctx->text_height(font, size);
  int clipped = mu_check_clip(ctx, &rect);
  if (clipped == MU_CLIP_ALL ) { return; }
  if (clipped == MU_CLIP_PART) { mu_set_clip(ctx, mu_get_clip_rect(ctx)); }  
  /* add command */
  if (len < 0) { len = strlen(str); }
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MU_COMMAND_TEXT: pos=[%d,%d], %s[%d]", pos->x, pos->y, str, len);
  cmd = mu_push_command(ctx, MU_COMMAND_TEXT, sizeof(mu_TextCommand) + len);
  //memcpy(cmd->text.str, str, len);
  //cmd->text.str[len] = '\0';
  memmove((uint8_t*)cmd->text.str, str, len);
  memset ((uint8_t*)(cmd->text.str+len), 0, 1);
  //cmd->text.pos = pos;
  memmove((uint8_t*)&cmd->text.pos, (uint8_t*)pos, sizeof(mu_Vec2));
  //cmd->text.color = color;
  copy_color(&cmd->text.color, (const mu_Color*)color);
  //cmd->text.font = font;
  if (font != 0) memmove((uint8_t*)&cmd->text.font, (uint8_t*)font, sizeof(mu_Font));
  cmd->text.size = size;
  /* reset clipping if it was set */
  if (clipped) { mu_set_clip(ctx, &unclipped_rect); }
}


void mu_draw_icon(mu_Context *ctx, uint16_t id, mu_Rect *rect, mu_Color *color) {
  mu_Command *cmd;
  /* do clip command if the rect isn't fully contained within the cliprect */
  int clipped = mu_check_clip(ctx, rect);
  if (clipped == MU_CLIP_ALL ) { return; }
  if (clipped == MU_CLIP_PART) { mu_set_clip(ctx, mu_get_clip_rect(ctx)); }
  /* do icon command */
  //addLogAdv(LOG_INFO, LOG_FEATURE_DRV, "MU_COMMAND_ICON: rect=[%d,%d,%d,%d]", rect->x, rect->y, rect->w, rect->h);
  //rtos_delay_milliseconds(500);
  cmd = mu_push_command(ctx, MU_COMMAND_ICON, sizeof(mu_IconCommand));
  //cmd->icon.id = id;
   memmove((uint8_t*)&cmd->icon.id, &id, sizeof(uint16_t));
  //cmd->icon.rect = rect;
  //cmd->icon.color = color;
  copy_rect(&cmd->icon.rect, (const mu_Rect *)rect);
  copy_color(&cmd->icon.color, (const mu_Color*)color);
  /* reset clipping if it was set */
  if (clipped) { mu_set_clip(ctx, &unclipped_rect); }
}


/*============================================================================
** layout
**============================================================================*/

enum { RELATIVE = 1, ABSOLUTE = 2 };


void mu_layout_begin_column(mu_Context *ctx) {
  mu_Vec2 pos; pos.x = 0; pos.y = 0;
  mu_Rect rect; mu_layout_next(ctx, &rect);
  push_layout(ctx, &rect, &pos);
}


void mu_layout_end_column(mu_Context *ctx) {
  mu_Layout *a, *b;
  b = get_layout(ctx);
  pop(ctx->layout_stack);
  /* inherit position/next_row/max from child layout if they are greater */
  a = get_layout(ctx);
  a->position.x = mu_max(a->position.x, b->position.x + b->body.x - a->body.x);
  a->next_row = mu_max(a->next_row, b->next_row + b->body.y - a->body.y);
  a->max.x = mu_max(a->max.x, b->max.x);
  a->max.y = mu_max(a->max.y, b->max.y);
}


void mu_layout_row(mu_Context *ctx, int items, const int16_t *widths, int16_t height) {
  mu_Layout *layout = get_layout(ctx);
  if (widths) {
    expect(items <= MU_MAX_WIDTHS);
    memcpy(layout->widths, widths, items * sizeof(widths[0]));
  }
  layout->items = items;
  layout->position.x = layout->indent;
  layout->position.y = layout->next_row;
  layout->size.y = height;
  layout->item_index = 0;
}


void mu_layout_width(mu_Context *ctx, int16_t width) {
  get_layout(ctx)->size.x = width;
}


void mu_layout_height(mu_Context *ctx, int16_t height) {
  get_layout(ctx)->size.y = height;
}


void mu_layout_set_next(mu_Context *ctx, mu_Rect *rect, int relative) {
  mu_Layout *layout = get_layout(ctx);
  copy_rect(&layout->next, (const mu_Rect *)rect);
  layout->next_type = relative ? RELATIVE : ABSOLUTE;
}


void mu_layout_next(mu_Context *ctx, mu_Rect *res) {
  mu_Layout *layout = get_layout(ctx);
  mu_Style *style = ctx->style;

  if (layout->next_type) {
    /* handle rect set by `mu_layout_set_next` */
    int type = layout->next_type;
    layout->next_type = 0;
    copy_rect(res, &layout->next);
    if (type == ABSOLUTE) { 
		copy_rect(&ctx->last_rect, (const mu_Rect *)res);
		return; 
	}

  } else {
    /* handle next row */
    if (layout->item_index == layout->items) {
      mu_layout_row(ctx, layout->items, NULL, layout->size.y);
    }

    /* position */
    res->x = layout->position.x;
    res->y = layout->position.y;

    /* size */
    res->w = layout->items > 0 ? layout->widths[layout->item_index] : layout->size.x;
    res->h = layout->size.y;
    if (res->w == 0) { res->w = style->size.x + style->padding * 2; }
    if (res->h == 0) { res->h = style->size.y + style->padding * 2; }
    if (res->w <  0) { res->w += layout->body.w - res->x + 1; }
    if (res->h <  0) { res->h += layout->body.h - res->y + 1; }

    layout->item_index++;
  }

  /* update position */
  layout->position.x += res->w + style->spacing;
  layout->next_row = mu_max(layout->next_row, res->y + res->h + style->spacing);

  /* apply body offset */
  res->x += layout->body.x;
  res->y += layout->body.y;

  /* update max position */
  layout->max.x = mu_max(layout->max.x, res->x + res->w);
  layout->max.y = mu_max(layout->max.y, res->y + res->h);

  copy_rect(&ctx->last_rect, (const mu_Rect *)res);
}


/*============================================================================
** controls
**============================================================================*/

static uint8_t in_hover_root(mu_Context *ctx) {
  uint8_t i = ctx->container_stack.idx;
  while (i--) {
    if (ctx->container_stack.items[i] == ctx->hover_root) { return 1; }
    /* only root containers have their `head` field set; stop searching if we've
    ** reached the current root container */
    if (ctx->container_stack.items[i]->head) { break; }
  }
  return 0;
}


void mu_draw_control_frame(mu_Context *ctx, mu_Id id, mu_Rect *rect,
  uint8_t colorid, int opt)
{
  if (opt & MU_OPT_NOFRAME) { return; }
  colorid += (ctx->focus == id) ? 2 : (ctx->hover == id) ? 1 : 0;
  ctx->draw_frame(ctx, rect, colorid);
}


void mu_draw_control_text(mu_Context *ctx, const char *str, mu_Rect *rect,
  uint8_t colorid, int opt)
{
  mu_Vec2 pos;
  mu_Font *font = &ctx->style->font;
  uint8_t font_size = ctx->style->font_size;  
  int16_t tw = ctx->text_width(font, font_size, str, -1);
  mu_push_clip_rect(ctx, rect);
  pos.y = rect->y + (rect->h - ctx->text_height(font, font_size)) / 2;
  if (opt & MU_OPT_ALIGNCENTER) {
    pos.x = rect->x + (rect->w - tw) / 2;
  } else if (opt & MU_OPT_ALIGNRIGHT) {
    pos.x = rect->x + rect->w - tw - ctx->style->padding;
  } else {
    pos.x = rect->x + ctx->style->padding;
  }
  mu_draw_text(ctx, font, str, -1, &pos, &ctx->style->colors[colorid], font_size);
  mu_pop_clip_rect(ctx);
}


int mu_mouse_over(mu_Context *ctx, mu_Rect *rect) {  
  return rect_overlaps_vec2(rect, &ctx->mouse_pos) &&
    rect_overlaps_vec2(mu_get_clip_rect(ctx), &ctx->mouse_pos) &&
    in_hover_root(ctx);
}


void mu_update_control(mu_Context *ctx, mu_Id id, mu_Rect *rect, int opt) {
  int mouseover = mu_mouse_over(ctx, rect);

  if (ctx->focus == id) { ctx->updated_focus = 1; }
  if (opt & MU_OPT_NOINTERACT) { return; }
  if (mouseover && !ctx->mouse_down) { ctx->hover = id; }

  if (ctx->focus == id) {
    if (ctx->mouse_pressed && !mouseover) { mu_set_focus(ctx, 0); }
    if (!ctx->mouse_down && (opt & MU_OPT_HOLDFOCUS)) { mu_set_focus(ctx, 0); }
	if (ctx->key_pressed & MU_KEY_TAB) { mu_set_focus(ctx, 0); }
  } else if (ctx->focus == 0) {
	if (ctx->key_pressed & MU_KEY_TAB) { mu_set_focus(ctx, id); }
  }
  
  if (ctx->hover == id) {
    if (ctx->mouse_pressed) {
      mu_set_focus(ctx, id);
    } else if (!mouseover) {
      ctx->hover = 0;
    }
  }
}


void mu_text(mu_Context *ctx, const char *text) {
  const char *start, *end, *p = text;
  int16_t width = -1;
  mu_Font *font = &ctx->style->font;
  mu_Color *color = &ctx->style->colors[MU_COLOR_TEXT];
  uint8_t font_size = ctx->style->font_size;
  mu_layout_begin_column(ctx);
  mu_layout_row(ctx, 1, &width, ctx->text_height(font, font_size));
  do {
    mu_Rect r;
	mu_layout_next(ctx, &r);
    int w = 0;
    start = end = p;
    do {
      const char* word = p;
      while (*p && *p != ' ' && *p != '\n') { p++; }
      w += ctx->text_width(font, font_size, word, p - word);
      if (w > r.w && end != start) { break; }
      w += ctx->text_width(font, font_size, p, 1);
      end = p++;
    } while (*end && *end != '\n');
	mu_Vec2 pos = {.x=r.x, .y=r.y};	
    mu_draw_text(ctx, font, start, end - start, &pos, color, font_size);
    p = end + 1;
  } while (*end);
  mu_layout_end_column(ctx);
}


void mu_label(mu_Context *ctx, const char *text) {
  mu_Rect r;
  mu_layout_next(ctx, &r);
  mu_draw_control_text(ctx, text, &r, MU_COLOR_TEXT, 0);
}

void mu_label_ex(mu_Context *ctx, const char *text, mu_Color *color, uint8_t size, int opt) {
  mu_Rect rect;
  mu_layout_next(ctx, &rect);
  
  mu_Vec2 pos;
  mu_Font *font = &ctx->style->font;
  uint8_t font_size = ctx->style->font_size;
  if (size) font_size = size;
  int16_t tw = ctx->text_width(font, font_size, text, -1);
  mu_push_clip_rect(ctx, &rect);
  pos.y = rect.y + (rect.h - ctx->text_height(font, font_size)) / 2;
  if (opt & MU_OPT_ALIGNCENTER) {
    pos.x = rect.x + (rect.w - tw) / 2;
  } else if (opt & MU_OPT_ALIGNRIGHT) {
    pos.x = rect.x + rect.w - tw - ctx->style->padding;
  } else {
    pos.x = rect.x + ctx->style->padding;
  }
  mu_draw_text(ctx, font, text, -1, &pos, color, font_size);
  mu_pop_clip_rect(ctx);
}



int mu_button_ex(mu_Context *ctx, const char *label, int icon, int opt) {
  int res = 0;
  mu_Id id = label ? mu_get_id(ctx, label, strlen(label))
                   : mu_get_id(ctx, &icon, sizeof(icon));
  mu_Rect r;
  mu_layout_next(ctx, &r);
  mu_update_control(ctx, id, &r, opt);
  /* handle click */
  if (((ctx->mouse_pressed == MU_MOUSE_LEFT) || (ctx->key_pressed & MU_KEY_ENTER) ) && ctx->focus == id) {
    res |= MU_RES_SUBMIT;
  }
  /* draw */
  mu_draw_control_frame(ctx, id, &r, MU_COLOR_BUTTON, opt);  
  if (icon) { 
	mu_draw_icon(ctx, icon, &r, &ctx->style->colors[MU_COLOR_TEXT]); 
	r.x = r.x + (r.w < r.h ? r.w : r.h );
	r.w = r.w - ctx->style->padding - (r.w < r.h ? r.w : r.h );
  }
  if (label) { mu_draw_control_text(ctx, label, &r, MU_COLOR_TEXT, opt); }
  return res;
}


int mu_checkbox(mu_Context *ctx, const char *label, int *state) {
  int res = 0;
  mu_Id id = mu_get_id(ctx, &state, sizeof(state));
  mu_Rect r; mu_layout_next(ctx, &r);
  mu_Rect box; //copy_rect(&box, (const mu_Rect *)&r);
  box.x = r.x;
  box.y = r.y + (r.h/2) - 8;
  box.w = box.h = 16;
  mu_update_control(ctx, id, &r, 0);
  /* handle click */
  if (((ctx->mouse_pressed == MU_MOUSE_LEFT) || (ctx->key_pressed & MU_KEY_ENTER) ) && ctx->focus == id) {
    res |= MU_RES_CHANGE;
    *state = !*state;
  }
  /* draw */
  mu_draw_control_frame(ctx, id, &box, MU_COLOR_BASE, 0);
  if (*state) {
    mu_draw_icon(ctx, MU_ICON_CHECK, &box, &ctx->style->colors[MU_COLOR_TEXT]);
  }
  r.x = r.x + box.w; r.w = r.w - box.w;
  mu_draw_control_text(ctx, label, &r, MU_COLOR_TEXT, 0);
  return res;
}


int mu_textbox_raw(mu_Context *ctx, char *buf, int bufsz, mu_Id id, mu_Rect *rect, int opt)
{
  int res = 0;
  mu_update_control(ctx, id, rect, opt | MU_OPT_HOLDFOCUS);

  if (ctx->focus == id) {
    /* handle text input */
    int len = strlen(buf);
    int n = mu_min(bufsz - len - 1, (int) strlen(ctx->input_text));
    if (n > 0) {
      memcpy(buf + len, ctx->input_text, n);
      len += n;
      buf[len] = '\0';
      res |= MU_RES_CHANGE;
    }
    /* handle backspace */
    if (ctx->key_pressed & MU_KEY_BACKSPACE && len > 0) {
      /* skip utf-8 continuation bytes */
      while ((buf[--len] & 0xc0) == 0x80 && len > 0);
      buf[len] = '\0';
      res |= MU_RES_CHANGE;
    }
    /* handle return */
    if (ctx->key_pressed & MU_KEY_RETURN) {
      mu_set_focus(ctx, 0);
      res |= MU_RES_SUBMIT;
    }
  }

  /* draw */
  mu_draw_control_frame(ctx, id, rect, MU_COLOR_BASE, opt);
  if (ctx->focus == id) {
    mu_Color *color = &ctx->style->colors[MU_COLOR_TEXT];
    mu_Font *font = &ctx->style->font;
	uint8_t font_size = ctx->style->font_size;
	if (!font_size) font_size = 1;
    int16_t textw = ctx->text_width(font, font_size, buf, -1);
    int16_t texth = ctx->text_height(font, font_size);
    int16_t ofx = rect->w - ctx->style->padding - textw - 1;
    int16_t textx = rect->x + mu_min(ofx, ctx->style->padding);
    int16_t texty = rect->y + (rect->h - texth) / 2;
    mu_push_clip_rect(ctx, rect);
	mu_Vec2 pos = {.x=textx, .y=texty,};	
    mu_draw_text(ctx, font, buf, -1, &pos, color, font_size);
	mu_Rect rect = {.x=textx + textw, .y=texty, .w=1, .h=texth};
    mu_draw_rect(ctx, &rect, color);
    mu_pop_clip_rect(ctx);
  } else {
    mu_draw_control_text(ctx, buf, rect, MU_COLOR_TEXT, opt);
  }

  return res;
}


static int number_textbox(mu_Context *ctx, mu_Real *value, mu_Rect *r, mu_Id id) {
  if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->key_down & MU_KEY_SHIFT &&
      ctx->hover == id
  ) {
    ctx->number_edit = id;
    sprintf(ctx->number_edit_buf, MU_REAL_FMT, *value);
  }
  if (ctx->number_edit == id) {
    int res = mu_textbox_raw(
      ctx, ctx->number_edit_buf, sizeof(ctx->number_edit_buf), id, r, 0);
    if (res & MU_RES_SUBMIT || ctx->focus != id) {
      *value = strtod(ctx->number_edit_buf, NULL);
      ctx->number_edit = 0;
    } else {
      return 1;
    }
  }
  return 0;
}


int mu_textbox_ex(mu_Context *ctx, char *buf, int bufsz, int opt) {
  mu_Id id = mu_get_id(ctx, &buf, sizeof(buf));
  mu_Rect r; mu_layout_next(ctx, &r);
  return mu_textbox_raw(ctx, buf, bufsz, id, &r, opt);
}


int mu_slider_ex(mu_Context *ctx, mu_Real *value, mu_Real low, mu_Real high,
  mu_Real step, const char *fmt, int opt)
{
  char buf[MU_MAX_FMT + 1];
  mu_Rect thumb;
  int x, w, res = 0;
  mu_Real last = *value, v = last;
  mu_Id id = mu_get_id(ctx, &value, sizeof(value));
  mu_Rect base; mu_layout_next(ctx, &base);

  /* handle text input mode */
  if (number_textbox(ctx, &v, &base, id)) { return res; }

  /* handle normal mode */
  mu_update_control(ctx, id, &base, opt);

  /* handle input */
  if (ctx->focus == id &&
      (ctx->mouse_down | ctx->mouse_pressed) == MU_MOUSE_LEFT)
  {
    v = low + (ctx->mouse_pos.x - base.x) * (high - low) / base.w;
    if (step) { v = ((long long)((v + step / 2) / step)) * step; }
  }
  /* clamp and store value, update res */
  *value = v = mu_clamp(v, low, high);
  if (last != v) { res |= MU_RES_CHANGE; }

  /* draw base */
  mu_draw_control_frame(ctx, id, &base, MU_COLOR_BASE, opt);
  /* draw thumb */
  w = ctx->style->thumb_size;
  x = (v - low) * (base.w - w) / (high - low);
  thumb.x = base.x + x;
  thumb.y = base.y;
  thumb.w = w;
  thumb.h = base.h;
  mu_draw_control_frame(ctx, id, &thumb, MU_COLOR_BUTTON, opt);
  /* draw text  */
  sprintf(buf, fmt, v);
  mu_draw_control_text(ctx, buf, &base, MU_COLOR_TEXT, opt);

  return res;
}


int mu_number_ex(mu_Context *ctx, mu_Real *value, mu_Real step,
  const char *fmt, int opt)
{
  char buf[MU_MAX_FMT + 1];
  int res = 0;
  mu_Id id = mu_get_id(ctx, &value, sizeof(value));
  mu_Rect base; mu_layout_next(ctx, &base);
  mu_Real last = *value;

  /* handle text input mode */
  if (number_textbox(ctx, value, &base, id)) { return res; }

  /* handle normal mode */
  mu_update_control(ctx, id, &base, opt);

  /* handle input */
  if (ctx->focus == id && ctx->mouse_down == MU_MOUSE_LEFT) {
    *value += ctx->mouse_delta.x * step;
  }
  /* set flag if value changed */
  if (*value != last) { res |= MU_RES_CHANGE; }

  /* draw base */
  mu_draw_control_frame(ctx, id, &base, MU_COLOR_BASE, opt);
  /* draw text  */
  sprintf(buf, fmt, *value);
  mu_draw_control_text(ctx, buf, &base, MU_COLOR_TEXT, opt);

  return res;
}


static int header(mu_Context *ctx, const char *label, int istreenode, int opt) {
  mu_Rect r;
  int active, expanded;
  mu_Id id = mu_get_id(ctx, label, strlen(label));
  int idx = mu_pool_get(ctx, ctx->treenode_pool, MU_TREENODEPOOL_SIZE, id);
  int16_t width = -1;
  mu_layout_row(ctx, 1, &width, 0);

  active = (idx >= 0);
  /* handle click */
  active ^= (((ctx->mouse_pressed == MU_MOUSE_LEFT)||(ctx->key_pressed & MU_KEY_ENTER)) && ctx->focus == id);
  
  expanded = (opt & MU_OPT_EXPANDED) ? !active : active;
  mu_layout_next(ctx, &r);
  mu_update_control(ctx, id, &r, 0);


  /* update pool ref */
  if (idx >= 0) {
    if (active) { mu_pool_update(ctx, ctx->treenode_pool, idx); }
           else { memset(&ctx->treenode_pool[idx], 0, sizeof(mu_PoolItem)); }
  } else if (active) {
    mu_pool_init(ctx, ctx->treenode_pool, MU_TREENODEPOOL_SIZE, id);
  }

  /* draw */
  if (istreenode) {
    if (ctx->hover == id) { ctx->draw_frame(ctx, &r, MU_COLOR_BUTTONHOVER); }
  } else {
    mu_draw_control_frame(ctx, id, &r, MU_COLOR_BUTTON, 0);
  }
  mu_Rect r_icon;
  r_icon.x = r.x;
  r_icon.y = r.y;
  r_icon.h = r.h;
  r_icon.w = r.h;
  mu_draw_icon(
    ctx, expanded ? MU_ICON_EXPANDED : MU_ICON_COLLAPSED,
    &r_icon, &ctx->style->colors[MU_COLOR_TEXT]);
  r.x += r.h + ctx->style->padding;
  r.w -= r.h + ctx->style->padding;
  mu_draw_control_text(ctx, label, &r, MU_COLOR_TEXT, 0);

  return expanded ? MU_RES_ACTIVE : 0;
}


int mu_header_ex(mu_Context *ctx, const char *label, int opt) {
  return header(ctx, label, 0, opt);
}


int mu_begin_treenode_ex(mu_Context *ctx, const char *label, int opt) {
  int res = header(ctx, label, 1, opt);
  if (res & MU_RES_ACTIVE) {
    get_layout(ctx)->indent += ctx->style->indent;
    push(ctx->id_stack, ctx->last_id, mu_Id);
  }
  return res;
}


void mu_end_treenode(mu_Context *ctx) {
  get_layout(ctx)->indent -= ctx->style->indent;
  mu_pop_id(ctx);
}


#define scrollbar(ctx, cnt, b, cs, x, y, w, h)                              \
  do {                                                                      \
    /* only add scrollbar if content size is larger than body */            \
    int maxscroll = cs.y - b->h;                                            \
                                                                            \
    if (maxscroll > 0 && b->h > 0) {                                        \
      mu_Rect base, thumb;                                                  \
      mu_Id id = mu_get_id(ctx, "!scrollbar" #y, 11);                       \
                                                                            \
      /* get sizing / positioning */                                        \
      copy_rect(&base, (const mu_Rect *)b);                                 \
      base.x = b->x + b->w;                                                 \
      base.w = ctx->style->scrollbar_size;                                  \
                                                                            \
      /* handle input */                                                    \
      mu_update_control(ctx, id, &base, 0);                                 \
      if (ctx->focus == id && ctx->mouse_down == MU_MOUSE_LEFT) {           \
        cnt->scroll.y += ctx->mouse_delta.y * cs.y / base.h;                \
      }                                                                     \
      /* clamp scroll to limits */                                          \
      cnt->scroll.y = mu_clamp(cnt->scroll.y, 0, maxscroll);                \
                                                                            \
      /* draw base and thumb */                                             \
      ctx->draw_frame(ctx, &base, MU_COLOR_SCROLLBASE);                     \
      copy_rect(&thumb, (const mu_Rect *)&base);                            \
      thumb.h = mu_max(ctx->style->thumb_size, base.h * b->h / cs.y);       \
      thumb.y += cnt->scroll.y * (base.h - thumb.h) / maxscroll;            \
      ctx->draw_frame(ctx, &thumb, MU_COLOR_SCROLLTHUMB);                   \
                                                                            \
      /* set this as the scroll_target (will get scrolled on mousewheel) */ \
      /* if the mouse is over it */                                         \
      if (mu_mouse_over(ctx, b)) { ctx->scroll_target = cnt; }              \
    } else {                                                                \
      cnt->scroll.y = 0;                                                    \
    }                                                                       \
  } while (0)


static void scrollbars(mu_Context *ctx, mu_Container *cnt, mu_Rect *body) {
  int sz = ctx->style->scrollbar_size;
  mu_Vec2 cs = cnt->content_size;
  cs.x += ctx->style->padding * 2;
  cs.y += ctx->style->padding * 2;
  mu_push_clip_rect(ctx, body);
  /* resize body to make room for scrollbars */
  if (cs.y > cnt->body.h) { body->w -= sz; }
  if (cs.x > cnt->body.w) { body->h -= sz; }
  /* to create a horizontal or vertical scrollbar almost-identical code is
  ** used; only the references to `x|y` `w|h` need to be switched */
  scrollbar(ctx, cnt, body, cs, x, y, w, h);
  scrollbar(ctx, cnt, body, cs, y, x, h, w);
  mu_pop_clip_rect(ctx);
}


static void push_container_body(mu_Context *ctx, mu_Container *cnt, mu_Rect *body, int opt) {
  if (opt & MU_OPT_NOSCROLL) { scrollbars(ctx, cnt, body); }
  mu_Rect expanded_rect;
  expand_rect(body, -ctx->style->padding, &expanded_rect);
  push_layout(ctx, &expanded_rect, &cnt->scroll);
  copy_rect(&cnt->body, (const mu_Rect *)body);
}


static void begin_root_container(mu_Context *ctx, mu_Container *cnt) {
  push(ctx->container_stack, cnt, mu_Container *);
  /* push container to roots list and push head command */
  push(ctx->root_list, cnt, mu_Container *);
  cnt->head = push_jump(ctx, NULL);
  /* set as hover root if the mouse is overlapping this container and it has a
  ** higher zindex than the current hover root */
  if (rect_overlaps_vec2(&cnt->rect, &ctx->mouse_pos) &&
      (!ctx->next_hover_root || cnt->zindex > ctx->next_hover_root->zindex)
  ) {
    ctx->next_hover_root = cnt;
  }
  /* clipping is reset here in case a root-container is made within
  ** another root-containers's begin/end block; this prevents the inner
  ** root-container being clipped to the outer */
  push(ctx->clip_stack, unclipped_rect, mu_Rect);
}


static void end_root_container(mu_Context *ctx) {
  /* push tail 'goto' jump command and set head 'skip' command. the final steps
  ** on initing these are done in mu_end() */
  mu_Container *cnt = mu_get_current_container(ctx);
  cnt->tail = push_jump(ctx, NULL);
  cnt->head->jump.dst = ctx->command_list.items + ctx->command_list.idx;
  mu_pop_clip_rect(ctx);
  pop_container(ctx); 
}


int mu_begin_window_ex(mu_Context *ctx, const char *title, mu_Rect *rect, int opt) {
  mu_Rect body;
  mu_Id id = mu_get_id(ctx, title, strlen(title));
  
  mu_Container *cnt = get_container(ctx, id, opt);
  
  if (!cnt || !cnt->open) { return 0; }
  push(ctx->id_stack, id, mu_Id);
   
  if (cnt->rect.w == 0) { copy_rect(&cnt->rect, (const mu_Rect *)rect); }
  begin_root_container(ctx, cnt);
  copy_rect(rect, (const mu_Rect *)&cnt->rect);
  copy_rect(&body, (const mu_Rect *)&cnt->rect);  
  
  /* draw frame */
  if (!(opt & MU_OPT_NOFRAME)) {
    ctx->draw_frame(ctx, rect, MU_COLOR_WINDOWBG);
  }
  
  /* do title bar */
  if (!(opt & MU_OPT_NOTITLE)) {
    mu_Rect tr;
	copy_rect(&tr, (const mu_Rect *)rect);
    tr.h = ctx->style->title_height;
    ctx->draw_frame(ctx, &tr, MU_COLOR_TITLEBG);
	
    /* do title text */
    if (title) {		
      mu_Id id = mu_get_id(ctx, "!title", 6);
      mu_update_control(ctx, id, &tr, opt);
      mu_draw_control_text(ctx, title, &tr, MU_COLOR_TITLETEXT, opt);
      if (id == ctx->focus && ctx->mouse_down == MU_MOUSE_LEFT) {
        cnt->rect.x += ctx->mouse_delta.x;
        cnt->rect.y += ctx->mouse_delta.y;
      }
      body.y += tr.h;
      body.h -= tr.h;  
    }
	
    /* do `close` button */
    if (!(opt & MU_OPT_NOCLOSE)) {		
      mu_Id id = mu_get_id(ctx, "!close", 6);
      mu_Rect r;
	  r.x = tr.x + tr.w - tr.h;
	  r.y = tr.y;
	  r.w = tr.h;
      r.h = tr.h;
      tr.w -= r.w;
      mu_draw_icon(ctx, MU_ICON_CLOSE, &r, &ctx->style->colors[MU_COLOR_TITLETEXT]);
      mu_update_control(ctx, id, &r, opt);
      if (ctx->mouse_pressed == MU_MOUSE_LEFT && id == ctx->focus) {
        cnt->open = 0;
      }
    }
  }
  
  push_container_body(ctx, cnt, &body, opt);
  
  /* do `resize` handle */
  if (!(opt & MU_OPT_NORESIZE)) {
    int sz = ctx->style->title_height;
    mu_Id id = mu_get_id(ctx, "!resize", 7);
    mu_Rect r;
	r.x = rect->x + rect->w - sz;
    r.y = rect->y + rect->h - sz;
	r.w = sz;
	r.h = sz;
    mu_update_control(ctx, id, &r, opt);
    if (id == ctx->focus && ctx->mouse_down == MU_MOUSE_LEFT) {
      cnt->rect.w = mu_max(96, cnt->rect.w + ctx->mouse_delta.x);
      cnt->rect.h = mu_max(64, cnt->rect.h + ctx->mouse_delta.y);
    }
  }
  
  /* resize to content size */
  if (opt & MU_OPT_AUTOSIZE) {
    mu_Rect r;
	copy_rect(&r, (const mu_Rect *)&get_layout(ctx)->body);
    cnt->rect.w = cnt->content_size.x + (cnt->rect.w - r.w);
    cnt->rect.h = cnt->content_size.y + (cnt->rect.h - r.h);
  }
  
  /* close if this is a popup window and elsewhere was clicked */
  if (opt & MU_OPT_POPUP && ctx->mouse_pressed && ctx->hover_root != cnt) {
    cnt->open = 0;
  }  
  
  mu_push_clip_rect(ctx, &cnt->body);
  return MU_RES_ACTIVE;
}


void mu_end_window(mu_Context *ctx) {
  mu_pop_clip_rect(ctx);
  end_root_container(ctx);
}


void mu_open_popup(mu_Context *ctx, const char *name) {
  mu_Container *cnt = mu_get_container(ctx, name);
  /* set as hover root so popup isn't closed in begin_window_ex()  */
  ctx->hover_root = ctx->next_hover_root = cnt;
  /* position at mouse cursor, open and bring-to-front */  
  cnt->rect.x = ctx->mouse_pos.x;
  cnt->rect.y = ctx->mouse_pos.y;
  cnt->rect.w = 1;
  cnt->rect.h = 1;
  cnt->open = 1;
  mu_bring_to_front(ctx, cnt);
}


int mu_begin_popup(mu_Context *ctx, const char *name) {
  int opt = MU_OPT_POPUP | MU_OPT_AUTOSIZE | MU_OPT_NORESIZE |
            MU_OPT_NOSCROLL | MU_OPT_NOTITLE | MU_OPT_CLOSED;
  mu_Rect rect = {.x=0, .y=0, .w=0, .h=0};
  return mu_begin_window_ex(ctx, name, &rect, opt);
}


void mu_end_popup(mu_Context *ctx) {
  mu_end_window(ctx);
}


void mu_begin_panel_ex(mu_Context *ctx, const char *name, int opt) {
  mu_Container *cnt;
  mu_push_id(ctx, name, strlen(name));
  cnt = get_container(ctx, ctx->last_id, opt);
  mu_layout_next(ctx, &cnt->rect);
  if (opt & MU_OPT_NOFRAME) {
    ctx->draw_frame(ctx, &cnt->rect, MU_COLOR_PANELBG);
  }
  push(ctx->container_stack, cnt, mu_Container *);
  push_container_body(ctx, cnt, &cnt->rect, opt);
  mu_push_clip_rect(ctx, &cnt->body);
}


void mu_end_panel(mu_Context *ctx) {
  mu_pop_clip_rect(ctx);
  pop_container(ctx);
}

int8_t mu_is_intersect_rect( mu_Rect *r1,  mu_Rect *r2) {
	if ((r1->x >= (r2->x+r2->w-1)) || (r2->x >= (r1->x+r1->w-1)) || (r1->y >= (r2->y+r2->h-1)) || (r2->y >= (r1->y+r1->h-1))) return -1;
	if ((MAX(r1->x, r2->x) <= MIN((r1->x+r1->w-1), (r2->x+r2->w-1))) && (MAX(r1->y, r2->y) <= MIN(r1->y+r1->h-1, r2->y+r2->h-1))) return 0;
	else return -1;
}