/**
 * ============================================================================
 * BÀI HỌC & THỬ THÁCH EMBEDDED C - NGÀY 08 (Thời lượng: 60 - 75 phút - Độ khó: Khó)
 * Chủ đề: Bộ Đệm Vòng (Ring Buffer), Cửa Sổ Trượt TinyML & Máy Trạng Thái (FSM)
 * Tài liệu tra cứu chuẩn:
 *   - Chuẩn ngôn ngữ C:       https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git: https://git-scm.com/cheat-sheet
 * ============================================================================
 *
 * ============================================================================
 * PHẦN 1: CƠ CHẾ HOẠT ĐỘNG CỐT LÕI (KHÔNG CÓ CÔNG THỨC SẴN — TỰ SUY LUẬN)
 * ============================================================================
 *
 * 1. CƠ CHẾ BỘ ĐỆM VÒNG (RING BUFFER / CIRCULAR QUEUE)
 *    - Thay vì dịch mảng tốn thời gian mỗi khi lấy phần tử đầu ra, ta nối ảo
 *      ô cuối cùng của mảng tĩnh quay vòng về ô đầu tiên thành một vòng tròn.
 *    - Dùng hai mốc vị trí độc lập: một mốc cho bên GHI (Producer / Ngắt DMA)
 *      và một mốc cho bên ĐỌC (Consumer / Vòng lặp chính).
 *    - Vấn đề cốt lõi cần tự thiết kế: Làm thế nào để phân biệt trạng thái
 *      RỖNG và trạng thái ĐẦY mà vẫn tận dụng được 100% dung lượng mảng tĩnh.
 *
 * 2. CƠ CHẾ GHI/ĐỌC HÀNG LOẠT (BURST) & CỬA SỔ TRƯỢT TINYML (SLIDING WINDOW)
 *    - Khi ghi/đọc một khối nhiều byte, khối dữ liệu có thể bị cắt đôi qua
 *      ranh giới cuối mảng rồi vòng về đầu mảng.
 *    - Trong bài toán AI thời gian thực, khi cửa sổ đã đầy mà có mẫu mới đến,
 *      chế độ ghi đè (Overwrite) sẽ bỏ đi mẫu cũ nhất để luôn giữ đủ N mẫu
 *      mới nhất theo đúng thứ tự thời gian.
 *
 * 3. CƠ CHẾ MÁY TRẠNG THÁI HỮU HẠN (FINITE STATE MACHINE - FSM)
 *    - Khi dữ liệu đến từng byte một rời rạc, FSM chia quá trình giải mã thành
 *      các trạng thái rời rạc. Mỗi byte đến chỉ làm thay đổi trạng thái hiện tại
 *      và trả quyền điều khiển ngay lập tức (non-blocking).
 *    - Yêu cầu sống còn của FSM trong Firmware: Mọi sai lệch (byte rác, sai độ
 *      dài, sai kiểm tra toàn vẹn) đều phải tự phục hồi sạch sẽ về trạng thái
 *      chờ ban đầu.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * PHẦN 2: 5 BÀI TẬP THỰC HÀNH (TỰ THIẾT KẾ STRUCT, THUẬT TOÁN & CÔNG THỨC)
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * BÀI TẬP 1: BỘ ĐỆM VÒNG FIFO CHUẨN CÔNG NGHIỆP
 * Yêu cầu hành vi:
 *   - Tự thiết kế các trường bên trong `RingBuffer_t` (xóa biến `_replace_me`).
 *   - Bộ đệm hoạt động trên mảng tĩnh `storage` do bên ngoài cấp, phải tận dụng
 *     được ĐỦ `capacity` byte (không lãng phí ô trống).
 *   - `RB_Init`: Khởi tạo bộ đệm. Trả `false` nếu tham số không hợp lệ.
 *   - `RB_Push`: Ghi 1 byte. Từ chối (trả `false`) nếu đầy hoặc tham số sai.
 *   - `RB_Pop`:  Lấy 1 byte cũ nhất ra ngoài. Từ chối nếu rỗng hoặc tham số sai.
 *   - `RB_Peek`: Đọc xem trước byte cũ nhất nhưng KHÔNG xóa khỏi bộ đệm.
 *   - `RB_Count` / `RB_FreeSpace`: Trả về số byte đang chứa / số ô còn trống.
 * ---------------------------------------------------------------------------- */
typedef struct {
    /* TODO 1A: Xóa biến tạm bên dưới và tự thiết kế các trường quản lý RingBuffer */
    uint8_t *buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} RingBuffer_t;

bool RB_Init(RingBuffer_t *rb, uint8_t *storage, size_t capacity) {
    // TODO 1B
    if( capacity == 0 || rb == NULL || storage == NULL ) return false;
    rb->buffer = storage;
    rb->capacity = capacity;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
    return true;
}

bool RB_Push(RingBuffer_t *rb, uint8_t data) {
    // TODO 1C
    if( rb == NULL || rb->buffer == NULL  ) return false;
    if( rb->count == rb->capacity ) return false;
    rb->buffer[rb->head] = data;
    rb->head++;
    if (rb->head == rb->capacity) rb->head = 0;
    rb->count++;
    return true;
}

bool RB_Pop(RingBuffer_t *rb, uint8_t *out_data) {
    // TODO 1D
    if( rb == NULL || rb->buffer == NULL || out_data == NULL || rb->count == 0 ) return false;
    *out_data = rb->buffer[rb->tail];
    rb->tail++;
    if( rb->tail == rb->capacity ) rb->tail = 0;
    rb->count--;
    return true;
}

bool RB_Peek(const RingBuffer_t *rb, uint8_t *out_data) {
    // TODO 1E
    if( rb == NULL || rb->buffer == NULL || out_data == NULL || rb->count == 0 ) return false;
    *out_data = rb->buffer[rb->tail];
    return true;;
}

size_t RB_Count(const RingBuffer_t *rb) {
    // TODO 1F
    if( rb == NULL || rb->buffer == NULL || rb->count == 0 ) return 0U;
    return rb->count;
}

size_t RB_FreeSpace(const RingBuffer_t *rb) {
    // TODO 1G
    if( rb == NULL || rb->buffer == NULL ) return 0U;
    return ( (size_t)rb->capacity - rb->count);
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 2: GHI VÀ ĐỌC HÀNG LOẠT (BURST WRITE / BURST READ) QUA RANH GIỚI VÒNG
 * Yêu cầu hành vi:
 *   - `RB_WriteBurst`: Ghi tối đa `len` byte từ `src` vào `rb`. Nếu chỗ trống
 *     ít hơn `len`, chỉ ghi đúng số byte vừa khít chỗ trống. Trả về tổng số byte
 *     thực sự đã ghi.
 *   - `RB_ReadBurst`: Đọc tối đa `len` byte từ `rb` sang `dst`. Nếu dữ liệu hiện
 *     có ít hơn `len`, đọc hết số byte hiện có. Trả về tổng số byte thực sự đã đọc.
 *   - Cả hai hàm phải xử lý chính xác khi dữ liệu đi vòng qua cuối mảng về đầu mảng.
 * ---------------------------------------------------------------------------- */
size_t RB_WriteBurst(RingBuffer_t *rb, const uint8_t *src, size_t len) {
    // TODO 2A
    size_t count = 0; 
    if( rb == NULL || rb->buffer == NULL || src == NULL || len == 0 ) return 0U;
    for( int i = 0; i < len; i++){
        if(RB_Push(rb,src[i]) == false) break;
        else count++;
    }
    return count;
}

size_t RB_ReadBurst(RingBuffer_t *rb, uint8_t *dst, size_t len) {
    // TODO 2B
    if( rb == NULL || rb->buffer == NULL || dst == NULL || len == 0 ) return 0U;
    size_t count = 0;
    for( int i = 0; i < len; i++){
        if( RB_Pop(rb, &dst[i]) == false) break;
        else { count++;
        }
    }
    return count;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 3: CHẾ ĐỘ GHI ĐÈ & XUẤT CỬA SỔ TRƯỢT TINYML (SLIDING WINDOW)
 * Yêu cầu hành vi:
 *   - `RB_PushOverwrite`: Nếu chưa đầy thì ghi bình thường; nếu đã đầy thì ghi đè
 *     lên phần tử cũ nhất và dịch mốc đọc sao cho thứ tự thời gian luôn đúng.
 *   - `RB_SnapshotLinear`: Sao chép toàn bộ dữ liệu hiện có trong `rb` (tối đa
 *     `max_len` byte) ra mảng tuyến tính `dst` theo đúng thứ tự từ CŨ NHẤT đến
 *     MỚI NHẤT mà KHÔNG làm thay đổi hay xóa dữ liệu bên trong `rb`. Trả về số
 *     byte đã sao chép.
 * ---------------------------------------------------------------------------- */
bool RB_PushOverwrite(RingBuffer_t *rb, uint8_t data) {
    // TODO 3A
    if( rb == NULL || rb->buffer == NULL ) return false;
    if( rb->count < rb->capacity ) return RB_Push(rb,data);
    else {
        rb->buffer[rb->head] = data;
        rb->head++;
        if( rb->head == rb->capacity ) rb->head =0;
        rb->tail++;
        if( rb->tail == rb->capacity) rb->tail = 0;
    }
    return true;
}

size_t RB_SnapshotLinear(const RingBuffer_t *rb, uint8_t *dst, size_t max_len) {
    // TODO 3B
    if( rb == NULL || rb->buffer == NULL || dst == NULL || max_len == 0 || rb->count == 0) return 0U;
    size_t count = 0;
    size_t x = rb->tail;
    for (size_t i = 0; i < rb->count; i ++){
        dst[i] = rb->buffer[x];
        x++;
        if( x == rb->capacity ) x = 0;
        count++;
    }
    return count;
}


/* ----------------------------------------------------------------------------
 * BÀI TẬP 4: MÁY TRẠNG THÁI PHÂN TÍCH KHUNG TRUYỀN TỪNG BYTE (PARSER FSM)
 * Đặc tả khung truyền hợp lệ:
 *   - Byte mở đầu (Header): Cố định `0xAA`.
 *   - Byte độ dài tải (`L`): Chỉ hợp lệ trong đoạn `[1 .. 16]`.
 *   - Phần tải (Payload): Gồm đúng `L` byte dữ liệu.
 *   - Byte kiểm tra toàn vẹn (Checksum): Tổng XOR của Header, độ dài `L`, và
 *     toàn bộ `L` byte phần tải.
 * Yêu cầu hành vi:
 *   - Tự thiết kế các trạng thái và cấu trúc `FrameParser_t`.
 *   - `Parser_Init`: Đưa máy trạng thái về trạng thái chờ ban đầu.
 *   - `Parser_FeedByte`: Nạp từng byte rời rạc. Chỉ trả `true` và điền kết quả
 *     vào `out_frame` đúng tại byte cuối hoàn tất một khung hợp lệ, sau đó tự
 *     quay về chờ khung mới. Mọi trường hợp nhiễu/lỗi phải trả `false` và tự
 *     phục hồi sạch sẽ để bắt được khung hợp lệ kế tiếp.
 * ---------------------------------------------------------------------------- */
#define PARSER_HEADER_BYTE  (0xAAU)
#define PARSER_MAX_PAYLOAD  (16U)

typedef struct {
    uint8_t payload[PARSER_MAX_PAYLOAD];
    uint8_t length;
} DecodedFrame_t;

typedef enum {
    STATE_WAIT_HEADER = 0,  // Gán bắt đầu từ 0 (không gán cũng mặc định là 0)
    STATE_WAIT_LEN,         // Tự động tăng thành 1
    STATE_WAIT_PAYLOAD,     // Tự động tăng thành 2
    STATE_WAIT_CHECKSUM     // Tự động tăng thành 3
} ParserState_t; 

typedef struct {
    /* TODO 4A: Xóa biến tạm bên dưới và tự thiết kế trạng thái của FSM */
    ParserState_t state;
    uint8_t L ;
    uint8_t  payload[PARSER_MAX_PAYLOAD];
    uint8_t payload_idx;
    uint8_t checksum;
} FrameParser_t;

void Parser_Init(FrameParser_t *parser) {
    // TODO 4B
    if( parser == NULL ) return;
    parser->state = STATE_WAIT_HEADER;
    parser->L = 0;
    parser->payload_idx = 0;
    parser->checksum = 0;


}

bool Parser_FeedByte(FrameParser_t *parser, uint8_t byte, DecodedFrame_t *out_frame) {
    if (parser == NULL || out_frame == NULL) return false;

    switch (parser->state) {
        case STATE_WAIT_HEADER:
            if (byte == PARSER_HEADER_BYTE) {
                parser->checksum = byte;
                parser->state = STATE_WAIT_LEN;
            }
            break;

        case STATE_WAIT_LEN:
            if (byte == 0U || byte > PARSER_MAX_PAYLOAD) {
                if (byte == PARSER_HEADER_BYTE) {
                    parser->checksum = byte;
                    parser->state = STATE_WAIT_LEN;
                } else {
                    parser->state = STATE_WAIT_HEADER;
                }
            } else {
                parser->L = byte;
                parser->payload_idx = 0U;
                parser->checksum ^= byte;
                parser->state = STATE_WAIT_PAYLOAD;
            }
            break;

        case STATE_WAIT_PAYLOAD:
            parser->payload[parser->payload_idx++] = byte;
            parser->checksum ^= byte;
            if (parser->payload_idx == parser->L) {
                parser->state = STATE_WAIT_CHECKSUM;
            }
            break;

        case STATE_WAIT_CHECKSUM:
            parser->state = STATE_WAIT_HEADER;
            if (byte == parser->checksum) {
                out_frame->length = parser->L;
                for (size_t i = 0U; i < parser->L; i++) {
                    out_frame->payload[i] = parser->payload[i];
                }
                return true;
            }
            break;

        default:
            parser->state = STATE_WAIT_HEADER;
            break;
    }

    return false;
}

/* ----------------------------------------------------------------------------
 * BÀI TẬP 5: TÍCH HỢP PIPELINE (RING BUFFER + FSM + CHỐNG BÁO GIẢ DEBOUNCE)
 * Yêu cầu hành vi:
 *   - Tự thiết kế cấu trúc `AnomalyTrigger_t` và hàm `Pipeline_Init` lưu ngưỡng
 *     `threshold` và số gói liên tiếp cần thiết `required_streak`.
 *   - `Pipeline_ProcessStream`: Rút toàn bộ byte đang có trong `rb`, nạp qua
 *     `parser` để giải mã:
 *       + Mỗi khung hợp lệ (có ít nhất 1 byte tải): tăng `*out_valid_frames`
 *         (nếu con trỏ khác NULL) và kiểm tra giá trị `payload[0]`.
 *       + Nếu `payload[0]` LỚN HƠN HẲN `threshold`: tăng chuỗi bất thường liên
 *         tiếp; đạt đủ `required_streak` thì kích hoạt trạng thái báo động.
 *       + Nếu `payload[0]` NHỎ HƠN HOẶC BẰNG `threshold`: đặt chuỗi liên tiếp
 *         về 0 và tắt báo động.
 *       + Khung lỗi/nhiễu bị FSM loại bỏ KHÔNG được ảnh hưởng đến chuỗi đếm.
 *     Trả về trạng thái báo động hiện tại (`true` = đang báo động).
 * ---------------------------------------------------------------------------- */
typedef struct {
    uint8_t threshold;
    uint8_t required_streak;
    uint8_t current_streak;
    bool alarm_active;
} AnomalyTrigger_t;

void Pipeline_Init(AnomalyTrigger_t *det, uint8_t threshold, uint8_t required_streak) {
    if (det == NULL) return;
    det->threshold = threshold;
    det->required_streak = (required_streak == 0U) ? 1U : required_streak;
    det->current_streak = 0U;
    det->alarm_active = false;
}

bool Pipeline_ProcessStream(RingBuffer_t *rb,
                            FrameParser_t *parser,
                            AnomalyTrigger_t *det,
                            uint32_t *out_valid_frames) {
    if (rb == NULL || parser == NULL || det == NULL) return false;

    uint8_t byte = 0U;
    DecodedFrame_t frame;

    while (RB_Pop(rb, &byte)) {
        if (Parser_FeedByte(parser, byte, &frame)) {
            if (frame.length >= 1U) {
                if (out_valid_frames != NULL) {
                    (*out_valid_frames)++;
                }

                if (frame.payload[0] > det->threshold) {
                    if (det->current_streak < det->required_streak) {
                        det->current_streak++;
                    }
                    if (det->current_streak >= det->required_streak) {
                        det->alarm_active = true;
                    }
                } else {
                    det->current_streak = 0U;
                    det->alarm_active = false;
                }
            }
        }
    }

    return det->alarm_active;
}


/* ============================================================================
 * PHẦN 3: HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG NGÀY 08 (5 BÀI TẬP - KHÔNG SỬA HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0;

    printf("=== BAT DAU CHAM DIEM THU THACH NGAY 08 (RING BUFFER & FSM) ===\n\n");

    /* --- Test Bài tập 1: Ring Buffer FIFO cơ bản & Xoay vòng nhiều chu kỳ --- */
    uint8_t mem1[4] = {0};
    RingBuffer_t rb1;
    bool b1_ok = RB_Init(&rb1, mem1, 4U) && !RB_Init(&rb1, NULL, 4U) && !RB_Init(&rb1, mem1, 0U);
    b1_ok = b1_ok && RB_Push(&rb1, 10U) && RB_Push(&rb1, 20U) && RB_Push(&rb1, 30U) && RB_Push(&rb1, 40U);
    b1_ok = b1_ok && !RB_Push(&rb1, 50U); // Phải từ chối khi đã đủ 4/4 byte
    b1_ok = b1_ok && (RB_Count(&rb1) == 4U) && (RB_FreeSpace(&rb1) == 0U);

    uint8_t peek_v = 0U, pop_v = 0U;
    b1_ok = b1_ok && RB_Peek(&rb1, &peek_v) && (peek_v == 10U) && (RB_Count(&rb1) == 4U);
    b1_ok = b1_ok && RB_Pop(&rb1, &pop_v) && (pop_v == 10U) && (RB_Count(&rb1) == 3U);

    // Kiểm tra xoay vòng 1.000 lần liên tiếp
    bool wrap_ok = true;
    for (uint16_t i = 0; i < 1000U; i++) {
        uint8_t val = (uint8_t)(i & 0xFFU);
        uint8_t out = 0U;
        if (!RB_Push(&rb1, val) || !RB_Pop(&rb1, &out)) {
            wrap_ok = false;
            break;
        }
    }
    if (b1_ok && wrap_ok && (RB_Count(&rb1) == 3U) && (RB_FreeSpace(&rb1) == 1U)) {
        printf("[PASS] Bai tap 1: RingBuffer FIFO hoat dong on dinh qua 1000 chu ky!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 1: b1_ok=%d, wrap_ok=%d, count=%zu, free=%zu\n",
               b1_ok, wrap_ok, RB_Count(&rb1), RB_FreeSpace(&rb1));
    }

    /* --- Test Bài tập 2: Burst Write / Read cắt ngang ranh giới mảng --- */
    uint8_t mem2[6] = {0};
    RingBuffer_t rb2;
    RB_Init(&rb2, mem2, 6U);
    const uint8_t in_a[4] = {1, 2, 3, 4};
    uint8_t out_a[4] = {0};
    size_t w1 = RB_WriteBurst(&rb2, in_a, 4U); // Ghi 4 byte [0..3]
    size_t r1 = RB_ReadBurst(&rb2, out_a, 3U); // Đọc 3 byte -> còn 1 byte (4) ở vị trí [3]

    const uint8_t in_b[6] = {5, 6, 7, 8, 9, 10}; // Chỉ còn 5 chỗ trống -> phải ghi đúng 5 byte vòng qua cuối mảng
    size_t w2 = RB_WriteBurst(&rb2, in_b, 6U);
    uint8_t out_b[6] = {0};
    size_t r2 = RB_ReadBurst(&rb2, out_b, 6U);

    bool b2_seq = (out_a[0] == 1) && (out_a[1] == 2) && (out_a[2] == 3) &&
                  (out_b[0] == 4) && (out_b[1] == 5) && (out_b[2] == 6) &&
                  (out_b[3] == 7) && (out_b[4] == 8) && (out_b[5] == 9);
    if ((w1 == 4U) && (r1 == 3U) && (w2 == 5U) && (r2 == 6U) && b2_seq && (RB_Count(&rb2) == 0U)) {
        printf("[PASS] Bai tap 2: Burst Write/Read cat ngang ranh gioi mang chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 2: w1=%zu(4), r1=%zu(3), w2=%zu(5), r2=%zu(6), seq=%d\n",
               w1, r1, w2, r2, b2_seq);
    }

    /* --- Test Bài tập 3: Chế độ Ghi đè & Snapshot Cửa sổ Trượt TinyML --- */
    uint8_t mem3[4] = {0};
    RingBuffer_t rb3;
    RB_Init(&rb3, mem3, 4U);
    // Đẩy liên tục 7 mẫu: 10, 20, 30, 40, 50, 60, 70 -> 4 mẫu mới nhất phải là [40, 50, 60, 70]
    for (uint8_t v = 10U; v <= 70U; v += 10U) {
        RB_PushOverwrite(&rb3, v);
    }
    uint8_t snap[4] = {0};
    size_t n_snap = RB_SnapshotLinear(&rb3, snap, 4U);
    bool b3_ok = (n_snap == 4U) && (RB_Count(&rb3) == 4U) &&
                 (snap[0] == 40U) && (snap[1] == 50U) && (snap[2] == 60U) && (snap[3] == 70U);
    if (b3_ok) {
        printf("[PASS] Bai tap 3: Overwrite & Snapshot cua so truot TinyML [%u %u %u %u] chinh xac!\n",
               snap[0], snap[1], snap[2], snap[3]);
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 3: n_snap=%zu, snap=[%u %u %u %u] (ky vong 40 50 60 70)\n",
               n_snap, snap[0], snap[1], snap[2], snap[3]);
    }

    /* --- Test Bài tập 4: Máy trạng thái Parser FSM --- */
    FrameParser_t parser4;
    Parser_Init(&parser4);
    DecodedFrame_t frm = {0};
    // Luồng thử nghiệm gồm: rác -> gói hợp lệ 1 -> gói sai độ dài -> gói sai XOR -> gói hợp lệ 2
    const uint8_t stream4[] = {
        0x11U, 0x22U,                             // Byte rác trước Header
        0xAAU, 0x02U, 0x10U, 0x20U, 0x98U,        // Gói hợp lệ 1: len=2, [0x10, 0x20]
        0xAAU, 0x00U,                             // Lỗi: độ dài = 0
        0xAAU, 0x01U, 0x55U, 0x00U,               // Lỗi: sai Checksum XOR
        0xAAU, 0x03U, 0x01U, 0x02U, 0x03U, 0xA9U  // Gói hợp lệ 2: len=3, [0x01, 0x02, 0x03]
    };
    uint8_t valid_cnt4 = 0U;
    bool f1_ok = false, f2_ok = false;
    for (size_t i = 0; i < sizeof(stream4); i++) {
        if (Parser_FeedByte(&parser4, stream4[i], &frm)) {
            valid_cnt4++;
            if ((valid_cnt4 == 1U) && (frm.length == 2U) && (frm.payload[0] == 0x10U) && (frm.payload[1] == 0x20U)) {
                f1_ok = true;
            }
            if ((valid_cnt4 == 2U) && (frm.length == 3U) && (frm.payload[0] == 0x01U) && (frm.payload[2] == 0x03U)) {
                f2_ok = true;
            }
        }
    }
    if ((valid_cnt4 == 2U) && f1_ok && f2_ok) {
        printf("[PASS] Bai tap 4: Parser FSM loc nhieu, phuc hoi loi va giai ma 2 goi hop le chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 4: valid_cnt=%u (ky vong 2), f1_ok=%d, f2_ok=%d\n",
               valid_cnt4, f1_ok, f2_ok);
    }

    /* --- Test Bài tập 5: Pipeline tích hợp (RingBuffer + FSM + Chống báo giả) --- */
    uint8_t mem5[32] = {0};
    RingBuffer_t rb5;
    FrameParser_t parser5;
    AnomalyTrigger_t det5;
    RB_Init(&rb5, mem5, sizeof(mem5));
    Parser_Init(&parser5);
    Pipeline_Init(&det5, 100U, 3U); // Ngưỡng > 100, cần 3 gói hợp lệ liên tiếp mới báo động

    uint32_t total_valid = 0U;
    // Đợt 1: 120 (>100), 130 (>100), 50 (<=100 -> reset chuỗi về 0!)
    const uint8_t batch1[] = {
        0xAAU, 0x01U, 120U, 0xD3U,
        0xAAU, 0x01U, 130U, 0x29U,
        0xAAU, 0x01U,  50U, 0x99U
    };
    RB_WriteBurst(&rb5, batch1, sizeof(batch1));
    bool alarm1 = Pipeline_ProcessStream(&rb5, &parser5, &det5, &total_valid);

    // Đợt 2: 150 (chuỗi=1), gói lỗi XOR (phải bỏ qua, giữ chuỗi=1), 200 (chuỗi=2)
    const uint8_t batch2[] = {
        0xAAU, 0x01U, 150U, 0x3DU,
        0xAAU, 0x01U, 255U, 0x00U, // Gói nhiễu sai XOR
        0xAAU, 0x01U, 200U, 0x63U
    };
    RB_WriteBurst(&rb5, batch2, sizeof(batch2));
    bool alarm2 = Pipeline_ProcessStream(&rb5, &parser5, &det5, &total_valid);

    // Đợt 3: byte rác + 110 (chuỗi=3 -> KÍCH HOẠT BÁO ĐỘNG!)
    const uint8_t batch3[] = {
        0x77U, 0x88U,
        0xAAU, 0x01U, 110U, 0xC5U
    };
    RB_WriteBurst(&rb5, batch3, sizeof(batch3));
    bool alarm3 = Pipeline_ProcessStream(&rb5, &parser5, &det5, &total_valid);

    if (!alarm1 && !alarm2 && alarm3 && (total_valid == 6U) && (RB_Count(&rb5) == 0U)) {
        printf("[PASS] Bai tap 5: Pipeline RingBuffer + FSM + Chong bao gia hoat dong chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Bai tap 5: alarm=[%d %d %d] (ky vong 0 0 1), valid=%u (ky vong 6)\n",
               alarm1, alarm2, alarm3, (unsigned int)total_valid);
    }

    printf("\n=== KET QUA TONG KET NGAY 08: %u / 5 BAI TAP DAT [PASS] ===\n", pass_count);
    return 0;
}