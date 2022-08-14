#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/extensions/XTest.h>

#include "app.h"
#include "jskeycode2x11keycode.h"

typedef enum {
    jsmpeg_frame_type_video = 0xFA010000,
    jsmpeg_frame_type_audio = 0xFB010000
} jsmpeg_trame_type_t;

typedef struct {
    jsmpeg_trame_type_t type;
    int size;
    char data[0];
} jsmpeg_frame_t;

typedef struct {
    unsigned char magic[4];
    unsigned short width;
    unsigned short height;
} jsmpeg_header_t;


typedef enum {
    input_type_key = 0x0001,
    input_type_mouse_button = 0x0002,
    input_type_mouse_absolute = 0x0004,
    input_type_mouse_relative = 0x0008,
    input_type_mouse =
    input_type_mouse_button |
    input_type_mouse_absolute |
    input_type_mouse_relative
} input_type_t;

typedef struct {
    unsigned short type;
    unsigned short state;
    unsigned short keycode;
} input_key_t;

typedef struct {
    unsigned short type;
    unsigned short flags;
    float x, y;
    int amount; //For scrolling
} input_mouse_t;

typedef enum {
    key_down = 0x01,
    key_up = 0x00,
    mouse_1_down = 0x0002,
    mouse_1_up = 0x0004,
    mouse_2_down = 0x0008,
    mouse_2_up = 0x0010,
    mouse_scrolling = 0x0800
} input_state_t;


int swap_int32(int in) {
    return ((in>>24)&0xff) |
            ((in<<8)&0xff0000) |
            ((in>>8)&0xff00) |
            ((in<<24)&0xff000000);
}

int swap_int16(int in) {
    return ((in>>8)&0xff) | ((in<<8)&0xff00);
}

// Proxies for app_on_* callbacks
void on_connect(server_t *server, struct lws *socket) { app_on_connect((app_t *)server->user, socket); }
int on_http_req(server_t *server, struct lws *socket, char *request) { return app_on_http_req((app_t *)server->user, socket, request); }
void on_message(server_t *server, struct lws *socket, void *data, size_t len) { app_on_message((app_t *)server->user, socket, data, len); }
void on_close(server_t *server, struct lws *socket) { app_on_close((app_t *)server->user, socket); }

app_t *app_create(Display *display, Window window, int port, int bit_rate, int out_width, int out_height) {
    app_t *self = (app_t *)malloc(sizeof(app_t));
    memset(self, 0, sizeof(app_t));

    self->display = display;
    self->window = window;

    self->mouse_speed = APP_MOUSE_SPEED;
    self->grabber = grabber_create(display, window);

    if( !out_width ) { out_width = self->grabber->width; }
    if( !out_height ) { out_height = self->grabber->height; }
    if( !bit_rate ) { bit_rate = out_width * 1500; } // estimate bit rate based on output size

    self->encoder = encoder_create(
                self->grabber->width, self->grabber->height, // in size
                out_width, out_height, // out size
                bit_rate
                );

    self->server = server_create(port, APP_FRAME_BUFFER_SIZE);
    if( !self->server ) {
        //printf("Error: could not create Server; try using another port\n");
        return NULL;
    }

    self->server->on_connect = on_connect;
    self->server->on_http_req = on_http_req;
    self->server->on_message = on_message;
    self->server->on_close = on_close;
    self->server->user = self; // Set the app as user data, so we can access it in callbacks

    return self;
}

void app_destroy(app_t *self) {
    if( self == NULL ) { return; }

    encoder_destroy(self->encoder);
    grabber_destroy(self->grabber);
    server_destroy(self->server);
    free(self);
}

int app_on_http_req(app_t *self, struct lws *socket, char *request) {
    //printf("http request: %s\n", request);
    if( strcmp(request, "/") == 0 ) {
        lws_serve_http_file(socket, "client/index.html", "text/html; charset=utf-8", NULL, 0);
        return 1;
    }
    else if(strcmp(request, "/jsmpg.js") == 0 ) {
        lws_serve_http_file(socket, "client/jsmpg.js", "text/javascript; charset=utf-8", NULL, 0);
        return 1;
    }
    else if( strcmp(request, "/jsmpg-vnc.js") == 0 ) {
        lws_serve_http_file(socket, "client/jsmpg-vnc.js", "text/javascript; charset=utf-8", NULL, 0);
        return 1;
    }
    return 0;
}

void app_on_connect(app_t *self, struct lws *socket) {
    //printf("\nclient connected: %s\n", server_get_client_address(self->server, socket));

    jsmpeg_header_t header = {
        {'j','s','m','p'},
        swap_int16(self->encoder->out_width), swap_int16(self->encoder->out_height)
    };
    server_send(self->server, socket, &header, sizeof(header), server_type_binary);
}

void app_on_close(app_t *self, struct lws *socket) {
    //printf("\nclient disconnected: %s\n", server_get_client_address(self->server, socket));
}

void app_on_message(app_t *self, struct lws *socket, void *data, size_t len) {
    input_type_t type = (input_type_t)((unsigned short *)data)[0];

		
    if( type & input_type_key && len >= sizeof(input_key_t) ) {
        input_key_t *input = (input_key_t *)data;

        _Bool pressed = (input->state == key_down);
        KeyCode x11keycode = js_keycode_to_x11keycode(self->display, (unsigned short) input->keycode);

        //Skip if the JS keycode couldn't be bound to a X11 keycode
        if (x11keycode != 0) {
            XTestFakeKeyEvent(self->display, x11keycode, pressed, 0);
           
        }

    }
    else if( type & input_type_mouse_absolute ) {
        input_mouse_t *input = (input_mouse_t *)data;

        if( type & input_type_mouse_absolute ) {
            float scale_x = ((float) self->encoder->in_width / self->encoder->out_width);
            float scale_y = ((float) self->encoder->in_height / self->encoder->out_height);

            int x = (int) input->x * scale_x;
            int y = (int) input->y * scale_y;

            XWarpPointer(self->display, self->window, self->window, 0, 0, 0, 0, x, y);
        }

        if( type & input_type_mouse_relative ) {
            int x = (int) (input->x * self->mouse_speed);
            int y = (int) (input->y * self->mouse_speed);

            XWarpPointer(self->display, self->window, None, 0, 0, 0, 0, x, y);
        }

        if( type & input_type_mouse_button ) {

            if (input->flags & mouse_scrolling)
            {
                unsigned int button = input->amount >= 0 ? 4 : 5;
                XTestFakeButtonEvent(self->display, button, 1, CurrentTime);
                XTestFakeButtonEvent(self->display, button, 0, CurrentTime);
            }
            else
            {
                unsigned int button = (input->flags & (mouse_1_down | mouse_1_up)) ? 1 : 3;
                _Bool pressed = ((input->flags & (mouse_1_down | mouse_2_down)) != 0);

                XTestFakeButtonEvent(self->display, button, pressed, CurrentTime);
            }
        }

    }
}

void app_run(app_t *self, int target_fps) {
    jsmpeg_frame_t *frame = (jsmpeg_frame_t *)malloc(APP_FRAME_BUFFER_SIZE);
    frame->type = jsmpeg_frame_type_video;
    frame->size = 0;

    double
            fps = 60.0f,
            wait_time = (1000.0f/target_fps) - 1.5f;

    clock_t start, end;

    start = clock();

    while( 1 ) {
        double delta = time_since(start);
        if( delta > wait_time ) {
            fps = fps * 0.95f + 50.0f/delta;

            //Grab screen
            start = clock();
            void *pixels = grabber_grab(self->grabber);
            double grab_time = time_since(start);

            //Encode
            start = clock();
            size_t encoded_size = APP_FRAME_BUFFER_SIZE - sizeof(jsmpeg_frame_t);
            encoder_encode(self->encoder, pixels, frame->data, &encoded_size);

            //Broadcast
            if( encoded_size ) {
                frame->size = swap_int32(sizeof(jsmpeg_frame_t) + encoded_size);
                server_broadcast(self->server, frame, sizeof(jsmpeg_frame_t) + encoded_size, server_type_binary);
            }

            double encode_time = time_since(start);

            printf("fps:%3d (grabbing:%6.2fms, scaling/encoding:%6.2fms)\r", (int)fps, grab_time, encode_time);

            //Reset the clock
            start = clock();
        }


        server_update(self->server);
    }

    free(frame);
}

double time_since(clock_t start) {
    return (clock() - start) * 1000 / CLOCKS_PER_SEC;
}
