# Thư viện giao tiếp P2P theo Frame:
## Cấu trúc Frame:
```
/*
 * Frame truyền dữ liệu:
 * | PREAMBLE |  START  |  DEST_ADDR  |  SRC_ADDR  | FRAME_ID | FRAME_TYPE |  LEN  |  DATA[n]  | CHECKSUM |
 *   1 Byte      1 Byte    n Byte        n Byte      2 Byte      1 Byte      2 Byte    1 Byte     1 Byte
 */
 PREAMBLE = 0xAA
 START = 7E
 DEST_ADDR = Được tùy chỉnh bởi người dùng (Độ dài lớn nhất = BITS_PROTOCOL_ADDR_SIZE)
 SRC_ADDR = Được tùy chỉnh bởi người dùng (Độ dài lớn nhất = BITS_PROTOCOL_ADDR_SIZE)
 FRAME_ID = Tự động tăng mỗi Frame
 FRAME_TYPE = BITS_Protocol_Type_t
 LEN = Độ dài của DATA
 DATA = Dữ liệu người dùng
 CHECKSUM = Tính từ DEST_ADDR đến hết DATA
```
## Các chức năng chính:
- Truyền dữ liệu giữa 2 hoặc nhiều thiết bị.
- Trả về ACK cho mỗi gói tin cần xác thực.
- Tự gửi lại gói tin nếu bị miss.
- Chu kỳ gửi lại được tính toán ngẫu nhiên.

## Cách thức hoạt động:
- ```BITS_Protocol_Parser```  Giải mã các dữ liệu đã nhận được. Gửi ACK cho các device cần xác thực gói tin, nhận ACK xác thực các gói tin đã gửi.
- ```BITS_Protocol_Exe```  Gửi các gói tin trong hàng chờ, xóa những gói tin đã hết số lần retry. Reset bộ giải mã nếu hết thời gian nhận Frame.
- ```BITS_Protocol_SendData```  Tạo frame và đưa dữ liệu vào hàng chờ chuẩn bị gửi đi.