/**
 * ============================================================================
 * BÀI HỌC & THỬ THÁCH EMBEDDED C - NGÀY 05 (Thời lượng: 60 phút)
 * Chủ đề: Memory Alignment & Padding trong `struct`, Kỹ thuật `union` đóng gói
 *         dữ liệu cảm biến & Tối ưu RAM cho Mô hình Embedded AI
 * Tài liệu tra cứu chuẩn:
 *   - Struct, Union & Alignment trong C: https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git hàng ngày:  https://git-scm.com/cheat-sheet
 * ============================================================================
 *
 * ============================================================================
 * PHẦN 1: BÀI HỌC LÝ THUYẾT CỐT LÕI (15 PHÚT ĐỌC HIỂU TRƯỚC KHI CODE)
 * ============================================================================
 *
 * 1. MEMORY ALIGNMENT & PADDING — TẠI SAO `sizeof(struct)` LẠI LỚN HƠN TỔNG CÁC BIẾN?
 * ----------------------------------------------------------------------------
 * Vi điều khiển 32-bit (như ARM Cortex-M trên STM32 hay ESP32-S3) đọc dữ liệu
 * từ RAM hiệu quả nhất khi địa chỉ của biến chia hết cho kích thước của nó:
 *   - `uint8_t`  (1 byte): đặt ở địa chỉ bất kỳ.
 *   - `uint16_t` (2 byte): đặt ở địa chỉ chẵn (chia hết cho 2).
 *   - `uint32_t` / `float` (4 byte): đặt ở địa chỉ chia hết cho 4.
 *
 * Khi bạn khai báo `struct` lộn xộn:
 *     typedef struct {
 *         uint8_t  flag;   // 1 byte (+ 3 byte ĐỆM RÁC để `val` nằm ở địa chỉ chia hết cho 4)
 *         uint32_t val;    // 4 byte
 *         uint8_t  status; // 1 byte (+ 3 byte ĐỆM RÁC ở cuối để cả struct chia hết cho 4)
 *     } BadStruct_t;       // Tổng tốn 12 BYTE thay vì 6 byte!
 *
 * -> TẠI SAO CỰC KỲ QUAN TRỌNG VỚI EMBEDDED AI?
 *    Nếu bạn lưu 1.000 mẫu dữ liệu cảm biến (window buffer) bằng `BadStruct_t`,
 *    bạn lãng phí mất 4.000 byte RAM (~4 KB) vô ích!
 * -> QUY TẮC TỐI ƯU: Luôn sắp xếp các biến trong `struct` theo thứ tự
 *    KÍCH THƯỚC GIẢM DẦN (`uint32_t`/`float` -> `uint16_t` -> `uint8_t`/`bool`).
 *
 * 2. KỸ THUẬT `union` KẾT HỢP `struct` — NHÌN 1 VÙNG RAM BẰNG 2 GÓC NHÌN
 * ----------------------------------------------------------------------------
 * Trong `struct`, các biến nằm nối tiếp nhau. Trong `union`, tất cả các thành phần
 * NẰM ĐÈ LÊN CÙNG MỘT ĐỊA CHỈ RAM (kích thước của `union` = kích thước thành phần lớn nhất).
 *
 * Ứng dụng kinh điển trong Nhúng & TinyML:
 *   (A) Bóc tách 1 số thực `float` (4 byte chuẩn IEEE-754) thành 4 byte `uint8_t`
 *       để gửi qua UART/SPI mà không cần ép kiểu con trỏ nguy hiểm:
 *       typedef union {
 *           float   f_val;       // Góc nhìn 1: Số thực 32-bit
 *           uint8_t bytes[4];    // Góc nhìn 2: Mảng 4 byte thô
 *       } FloatBytes_u;
 *   (B) Đóng gói 1 khung truyền (Frame) gồm nhiều trường vào chung với mảng `uint8_t raw[N]`
 *       để tính Checksum hoặc đẩy thẳng vào bộ đệm DMA chỉ trong 1 vòng lặp `for`.
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * PHẦN 2: 4 THỬ THÁCH THỰC HÀNH NGÀY 05 (45 PHÚT TỰ SUY NGHĨ & CODE)
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * THỬ THÁCH 1: TỐI ƯU BỐ CỤC `struct` ĐỂ GIẢM RAM TỪ 20 BYTE XUỐNG 12 BYTE
 * Đặc tả kỹ thuật:
 *   - Nhìn `AI_Sample_Bad_t` bên dưới: chứa 6 biến (tổng dữ liệu thật = 11 byte),
 *     nhưng do sắp xếp lộn xộn nên trình biên dịch chèn đệm rác phình lên tới 20 byte!
 *   - Nhiệm vụ của bạn: Giữ nguyên TÊN và KIỂU DỮ LIỆU của cả 6 biến đó, nhưng
 *     sắp xếp lại thứ tự bên trong `AI_Sample_Opt_t` sao cho `sizeof(AI_Sample_Opt_t)`
 *     nhỏ hơn hoặc bằng 12 byte!
 * ---------------------------------------------------------------------------- */
typedef struct {
    uint8_t  channel_id;    // 1 byte
    uint32_t timestamp_ms;  // 4 byte
    uint8_t  label_id;      // 1 byte
    int16_t  accel_z;       // 2 byte
    bool     is_valid;      // 1 byte
    int16_t  gyro_z;        // 2 byte
} AI_Sample_Bad_t;

typedef struct {
    // TODO 1: Sắp xếp lại đúng 6 trường của AI_Sample_Bad_t theo thứ tự tối ưu RAM nhất
    uint32_t timestamp_ms;
    int16_t  accel_z;
    int16_t  gyro_z;
    uint8_t  channel_id;
    uint8_t  label_id;
    bool     is_valid;
} AI_Sample_Opt_t;


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 2: TÁCH & GHÉP SỐ THỰC `float` (32-BIT) QUA MẢNG 4 BYTE BẰNG `union`
 * Đặc tả kỹ thuật:
 *   - Trong hệ thống nhúng AI, nhiều cảm biến hoặc trọng số mô hình truyền qua
 *     UART dưới dạng 4 byte của số thực `float`.
 *   - Hãy định nghĩa một `union` tên là `FloatConverter_u` gồm 2 thành phần:
 *       + `float f_val;`
 *       + `uint8_t bytes[4];`
 *   - Viết 2 hàm sử dụng `FloatConverter_u`:
 *       + `Float_ToBytes(float input, uint8_t out_buf[4])`: Nếu `out_buf != NULL`,
 *         chép 4 byte của `input` vào `out_buf[0..3]`.
 *       + `Bytes_ToFloat(const uint8_t in_buf[4])`: Nếu `in_buf == NULL`, trả về `0.0f`.
 *         Ngược lại, ghép 4 byte từ `in_buf[0..3]` thành số `float` và trả về.
 * ---------------------------------------------------------------------------- */

/* TODO 2A: Khai báo typedef union { ... } FloatConverter_u; tại đây */
typedef union {
    float f_val;
    uint8_t bytes[4];
}FloatConverter_u;

void Float_ToBytes(float input, uint8_t out_buf[4]) {
    // TODO 2B: Dùng FloatConverter_u để tách `input` ra mảng `out_buf[0..3]
    if( out_buf != NULL){
        FloatConverter_u conv;
        conv.f_val = input;
        for(int i = 0; i < 4; i++){
            out_buf[i] = conv.bytes[i];
        }
    }
}

float Bytes_ToFloat(const uint8_t in_buf[4]) {
    // TODO 2C: Dùng FloatConverter_u để ghép `in_buf[0..3]` thành số `float`
    FloatConverter_u conv;
    if( in_buf == NULL) return 0.0f;
    else {
        
        for( int i = 0; i < 4; i++){
            conv.bytes[i] = in_buf[i];
        }
    }
    return conv.f_val;
}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 3: ĐÓNG GÓI KHUNG DỮ LIỆU TINYML 8 BYTE BẰNG `union` & TÍNH CHECKSUM
 * Đặc tả khung truyền `TinyML_Frame_u` (Đúng 8 byte, không có byte đệm rác):
 *   - Byte 0    : `header`     (uint8_t) - Luôn bằng 0x55
 *   - Byte 1    : `class_id`   (uint8_t) - Nhãn dự đoán AI (0..255)
 *   - Byte 2..3 : `confidence` (uint16_t)- Độ tin cậy 0..10000 (ví dụ 9550 = 95.50%)
 *   - Byte 4..5 : `latency_us` (uint16_t)- Thời gian suy luận AI tính bằng micro-giây
 *   - Byte 6    : `flags`      (uint8_t) - Thanh ghi cờ trạng thái
 *   - Byte 7    : `checksum`   (uint8_t) - Tổng XOR (`^`) của 7 byte đầu `raw[0..6]`
 *
 * Yêu cầu hàm `TinyML_BuildFrame`:
 *   - Nếu `frame == NULL`, thoát hàm ngay.
 *   - Điền đầy đủ 5 trường (`header = 0x55`, `class_id`, `confidence`, `latency_us`, `flags`)
 *     vào `frame->fields`.
 *   - Duyệt vòng lặp `for` trên `frame->raw[0..6]` để tính XOR và gán vào `frame->fields.checksum`.
 * ---------------------------------------------------------------------------- */
typedef union {
    struct {
        uint8_t  header;
        uint8_t  class_id;
        uint16_t confidence;
        uint16_t latency_us;
        uint8_t  flags;
        uint8_t  checksum;
    } fields;
    uint8_t raw[8];
} TinyML_Frame_u;

void TinyML_BuildFrame(TinyML_Frame_u *frame,
                       uint8_t class_id,
                       uint16_t confidence,
                       uint16_t latency_us,
                       uint8_t flags) {
    // TODO 3: Điền dữ liệu vào frame->fields và tính Checksum XOR qua mảng frame->raw[0..6]
    if( frame == NULL) return;
    
    frame->fields.header = 0x55;
    frame->fields.class_id = class_id;
    frame->fields.confidence = confidence;
    frame->fields.flags = flags;
    frame->fields.latency_us = latency_us;
    for( int i = 0; i < 7; i++){
        frame->fields.checksum ^= frame->raw[i];
    }

}


/* ----------------------------------------------------------------------------
 * THỬ THÁCH 4: TỰ KIỂM TRA KIẾN THỨC THỨ TỰ BYTE CỦA CPU (ENDIANNESS)
 * Đặc tả kỹ thuật:
 *   - Viết hàm `System_IsLittleEndian(void)` sử dụng `union` gồm 1 biến `uint16_t val`
 *     và 1 mảng `uint8_t bytes[2]`.
 *   - Gán `val = 0x0001`.
 *   - Nếu máy tính/vi điều khiển theo chuẩn Little-Endian (như x86, ARM Cortex-M, ESP32),
 *     byte thấp `0x01` sẽ nằm ở địa chỉ đầu tiên `bytes[0]`.
 *   - Trả về `true` nếu `bytes[0] == 0x01`, ngược lại trả về `false`.
 * ---------------------------------------------------------------------------- */
typedef union{
    uint16_t val;
    uint8_t bytes[2];
}x;
bool System_IsLittleEndian(void) {
    // TODO 4: Dùng union để kiểm tra máy đang chạy là Little-Endian hay Big-Endian
    x y;
    y.val = 0x0001;
    if( y.bytes[0] == 0x01) return true;
    return false;
}


/* ============================================================================
 * PHẦN 3: HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG NGÀY 05 (KHÔNG SỬA ĐỔI HÀM MAIN)
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0;

    printf("=== BAT DAU CHAM DIEM BAI TAP NGAY 05 (STRUCT PADDING & UNION) ===\n\n");

    /* --- Test Thử thách 1 --- */
    size_t bad_sz = sizeof(AI_Sample_Bad_t);
    size_t opt_sz = sizeof(AI_Sample_Opt_t);
    if (opt_sz <= 12U) {
        printf("[PASS] Thu thach 1: Toi uu RAM thanh cong! Bad = %zu bytes -> Opt = %zu bytes\n", bad_sz, opt_sz);
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 1: sizeof(AI_Sample_Opt_t) = %zu bytes (can <= 12 bytes, Bad = %zu bytes)\n",
               opt_sz, bad_sz);
    }

    /* --- Test Thử thách 2 --- */
    uint8_t tx_buf[4] = {0};
    float original_f = -36.75f;
    Float_ToBytes(original_f, tx_buf);
    float recovered_f = Bytes_ToFloat(tx_buf);
    float null_f = Bytes_ToFloat(NULL);

    if ((recovered_f == original_f) && (null_f == 0.0f) &&
        ((tx_buf[0] != 0U) || (tx_buf[1] != 0U) || (tx_buf[2] != 0U) || (tx_buf[3] != 0U))) {
        printf("[PASS] Thu thach 2: Tach/ghep float %.2f -> [0x%02X 0x%02X 0x%02X 0x%02X] -> %.2f chinh xac!\n",
               original_f, tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3], recovered_f);
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 2: original=%.2f, recovered=%.2f\n", original_f, recovered_f);
    }

    /* --- Test Thử thách 3 --- */
    TinyML_Frame_u frame = {0};
    TinyML_BuildFrame(&frame, 2U, 9550U, 1420U, 0x05U);
    uint8_t expected_xor = 0U;
    for (uint8_t i = 0; i < 7U; i++) {
        expected_xor ^= frame.raw[i];
    }

    if ((sizeof(TinyML_Frame_u) == 8U) &&
        (frame.raw[0] == 0x55U) &&
        (frame.fields.class_id == 2U) &&
        (frame.fields.confidence == 9550U) &&
        (frame.fields.latency_us == 1420U) &&
        (frame.fields.flags == 0x05U) &&
        (frame.fields.checksum == expected_xor)) {
        printf("[PASS] Thu thach 3: Dong goi TinyML_Frame_u (8 bytes) & XOR=0x%02X chinh xac!\n",
               frame.fields.checksum);
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 3: header=0x%02X (0x55), xor=0x%02X (ky vong 0x%02X)\n",
               frame.raw[0], frame.fields.checksum, expected_xor);
    }

    /* --- Test Thử thách 4 --- */
    uint16_t probe = 0x0001U;
    bool actual_little_endian = (*((const uint8_t *)&probe) == 0x01U);
    bool user_res = System_IsLittleEndian();

    if (user_res == actual_little_endian) {
        printf("[PASS] Thu thach 4: System_IsLittleEndian() xac dinh chinh xac (%s)!\n",
               user_res ? "Little-Endian" : "Big-Endian");
        pass_count++;
    } else {
        printf("[FAIL] Thu thach 4: Ket qua tra ve %d khong khop voi kien truc CPU (%d)\n",
               user_res, actual_little_endian);
    }

    printf("\n=== KET QUA TONG KET NGAY 05: %u / 4 THU THACH DAT [PASS] ===\n", pass_count);
    return 0;
}