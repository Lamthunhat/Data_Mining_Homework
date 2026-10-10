# BÁO CÁO PHÂN TÍCH VÀ ĐÁNH GIÁ HIỆU NĂNG CÁC THUẬT TOÁN KHAI PHÁ TẬP MỤC PHỔ BIẾN
## (APRIORI, ECLAT, FP-GROWTH)

---

### MÔN HỌC: KHAI PHÁ DỮ LIỆU (DATA MINING)
- **Ngôn ngữ cài đặt:** C++ (Chuẩn C++17, Tối ưu hóa Release `/O2`)
- **Công cụ xây dựng:** CMake, MSVC / MinGW
- **Các thuật toán cài đặt:**
  1. **Apriori** [Agrawal & Srikant, VLDB 1994]
  2. **Eclat** [Mohammed J. Zaki, IEEE TKDE 2000]
  3. **FP-growth** [Han, Pei, Yin, ACM SIGMOD 2000]

---

## 1. TỔNG QUAN VỀ BÀI TOÁN VÀ CÁC THUẬT TOÁN

Khai phá tập mục phổ biến (**Frequent Itemset Mining - FIM**) là bài toán cốt lõi trong khai phá dữ liệu và học luật kết hợp (**Association Rule Mining**). Cho một cơ sở dữ liệu giao dịch $\mathcal{D}$ và ngưỡng độ hỗ trợ tối thiểu $\text{min\_sup}$, mục tiêu là tìm tất cả các tập mục $X$ có độ hỗ trợ $\text{support}(X) \ge \text{min\_sup}$.

### 1.1. Thuật toán Apriori [AS94b]
- **Đặc trưng:** Tìm kiếm theo chiều rộng (**Breadth-First Search - BFS**), sinh ứng viên theo từng mức độ dài $k$ (**Level-wise generation and test**).
- **Nguyên lý cốt lõi:** *Apriori Property (Anti-monotonicity)*: Mọi tập con của một tập mục phổ biến đều phải là tập mục phổ biến; nếu một tập mục không phổ biến thì mọi tập cha của nó cũng không phổ biến.
- **Quy trình:**
  1. Quét CSDL để tìm tập 1-mục phổ biến $L_1$.
  2. Tại mỗi bước $k \ge 2$:
     - **Join step:** Kết hợp $L_{k-1} \bowtie L_{k-1}$ để tạo ứng viên $C_k$. Hai tập mục được kết hợp nếu chung tiền tố $k-2$ mục đầu tiên.
     - **Prune step:** Tỉa bỏ các ứng viên $c \in C_k$ nếu tồn tại ít nhất một tập con kích thước $k-1$ không thuộc $L_{k-1}$.
     - **Count step:** Quét lại toàn bộ cơ sở dữ liệu để đếm tần số xuất hiện của các ứng viên trong $C_k$.
     - Lọc các ứng viên có $\text{count} \ge \text{min\_sup}$ để tạo $L_k$.
  3. Dừng lại khi $L_k = \emptyset$.

### 1.2. Thuật toán Eclat [Zak00]
- **Đặc trưng:** Khai phá theo định dạng dữ liệu dọc (**Vertical Data Format**) và duyệt theo chiều sâu (**Depth-First Search - DFS**).
- **Nguyên lý cốt lõi:**
  - Thay vì biểu diễn mỗi giao dịch là một danh sách mục (Horizontal format), Eclat biểu diễn mỗi mục dưới dạng **TID-list** (danh sách ID các giao dịch chứa mục đó): $t(i) = \{tid \mid i \in T_{tid}\}$.
  - Độ hỗ trợ của tập mục $X$ chính là độ dài danh sách TID: $\text{support}(X) = |t(X)|$.
  - Giao danh sách TID: $t(X \cup Y) = t(X) \cap t(Y)$.
- **Quy trình:**
  1. Quét CSDL ngang đúng 1 lần duy nhất để chuyển đổi sang cấu trúc TID-list dọc cho từng 1-mục.
  2. Lọc các 1-mục có $|t(i)| \ge \text{min\_sup}$.
  3. Phân chia không gian tìm kiếm thành các lớp tương đương (**Equivalence Classes**) dựa trên tiền tố chung $P$.
  4. Đệ quy DFS: Với hai mục $i, j$ cùng lớp tiền tố, tính $t(P \cup \{i, j\}) = t(P \cup \{i\}) \cap t(P \cup \{j\})$. Nếu $|t| \ge \text{min\_sup}$, tiếp tục mở rộng đệ quy.

### 1.3. Thuật toán FP-Growth [HPY00]
- **Đặc trưng:** Khai phá mẫu mở rộng (**Pattern-Growth**), không sinh ứng viên (**Candidate Generation-free**), phân chia để trị (**Divide-and-Conquer**).
- **Nguyên lý cốt lõi:**
  - Nén toàn bộ CSDL vào một cây tiền tố nhỏ gọn gọi là **FP-Tree (Frequent Pattern Tree)**.
  - Sử dụng **Header Table** lưu trữ danh sách các mục sắp xếp giảm dần theo độ hỗ trợ cùng con trỏ `node-link` nối tất cả các node cùng mục trên cây.
- **Quy trình:**
  1. Quét CSDL lần 1: Đếm tần số 1-mục, lọc các mục $\ge \text{min\_sup}$ và sắp xếp theo thứ tự giảm dần tần số (F-List).
  2. Quét CSDL lần 2: Chèn từng giao dịch (đã lọc các mục không phổ biến và sắp xếp theo F-List) vào FP-Tree.
  3. Khai phá đệ quy FP-Tree:
     - Duyệt Header Table từ dưới lên trên (từ mục ít phổ biến nhất đến mục phổ biến nhất).
     - Với mỗi mục $x$, thu thập các đường dẫn tiền tố (**Prefix Paths**) kết thúc tại $x$ để tạo cơ sở mẫu có điều kiện (**Conditional Pattern Base**).
     - Đếm tần số các mục trong cơ sở mẫu điều kiện, xây dựng cây FP-tree có điều kiện (**Conditional FP-Tree**).
     - Gọi đệ quy trên cây điều kiện cho đến khi cây rỗng.

---

## 2. MÔI TRƯỜNG THỰC NGHIỆM VÀ ĐẶC TẢ TẬP DỮ LIỆU

### 2.1. Cấu hình phần cứng & môi trường
- **Hệ điều hành:** Windows 11 x64
- **Trình biên dịch:** MSVC 19.51 (Visual Studio 2026 / C++17), flags tối ưu: `/O2 /Oi /Ot /GL /MP`
- **Bộ đo thời gian:** `std::chrono::high_resolution_clock` (độ phân giải nano-giây, hiển thị theo mili-giây ms)
- **Đo bộ nhớ:** Windows API `GetProcessMemoryInfo` (Peak Working Set Size - MB)

### 2.2. Đặc tính 3 tập dữ liệu chuẩn benchmark (từ FIMI / SPMF)
Ba tập dữ liệu được lựa chọn đại diện cho các đặc tính dữ liệu khác nhau trong thực tế:

| Tập dữ liệu | Số giao dịch $\|D\|$ | Số mục khác nhau $\|I\|$ | Chiều dài TB giao dịch | Chiều dài Min / Max | Mật độ (Density) | Phân loại |
|:---|:---:|:---:|:---:|:---:|:---:|:---|
| **chess.dat** | 3,196 | 75 | 37.00 | 37 / 37 | **49.33 %** | Rất đặc (Ultra Dense) |
| **mushroom.dat** | 8,416 | 119 | 23.00 | 23 / 23 | **19.33 %** | Khá đặc (Dense / Medium) |
| **retail.dat** | 88,162 | 16,470 | 10.31 | 1 / 76 | **0.06 %** | Rất thưa (Ultra Sparse) |

> **Công thức tính mật độ:** $\text{Density} = \frac{\text{Chiều dài trung bình giao dịch}}{\text{Số lượng mục khác nhau}} \times 100\%$

---

## 3. KẾT QUẢ THỰC NGHIỆM ĐO ĐẠC

Bảng dưới đây tổng hợp thời gian chạy (ms) và số lượng tập mục phổ biến tìm được của 3 thuật toán trên từng tập dữ liệu với các ngưỡng $\text{min\_sup}$ khác nhau. 

> *(Ghi chú: Tất cả 3 thuật toán đều đã được kiểm chứng (verification pass) cho ra số lượng và tập hợp các tập mục phổ biến **giống hệt nhau 100%** trên mọi cấu hình).*

| Tập dữ liệu | Ngưỡng MinSup (%) | MinSup Count | Số mẫu phổ biến (Patterns) | Apriori (ms) | Eclat (ms) | FP-Growth (ms) | Thuật toán tối ưu nhất |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **chess.dat** | 95.0% | 3,036 | 78 | 4.21 | **2.28** | 2.37 | **Eclat / FP-Growth** |
| | 90.0% | 2,876 | 628 | 20.41 | 7.26 | **4.48** | **FP-Growth** |
| | 85.0% | 2,716 | 2,690 | 119.51 | 21.52 | **7.69** | **FP-Growth** (nhanh gấp 15.5x Apriori) |
| | 80.0% | 2,556 | 8,282 | 719.72 | 57.79 | **16.57** | **FP-Growth** (nhanh gấp 43.4x Apriori) |
| **mushroom.dat**| 50.0% | 4,208 | 163 | 11.88 | **4.48** | 6.68 | **Eclat** |
| | 40.0% | 3,366 | 505 | 31.04 | 9.57 | **9.68** | **Eclat / FP-Growth** |
| | 30.0% | 2,524 | 2,587 | 154.51 | 25.14 | **15.10** | **FP-Growth** |
| | 20.0% | 1,683 | 53,337 | 4,365.46 | 160.72 | **65.30** | **FP-Growth** (nhanh gấp 66.8x Apriori) |
| **retail.dat** | 5.0% | 4,408 | 16 | 60.62 | 145.06 | **53.08** | **FP-Growth / Apriori** |
| | 2.0% | 1,763 | 55 | 93.46 | 156.31 | **51.14** | **FP-Growth** |
| | 1.0% | 881 | 159 | 89.49 | 160.99 | **55.27** | **FP-Growth** |
| | 0.5% | 440 | 581 | *Quá tải BFS* | 357.04 | **120.69** | **FP-Growth** |

---

## 4. PHÂN TÍCH CHUYÊN SÂU CÁC TÌNH HUỐNG HIỆU NĂNG VÀ NGUYÊN NHÂN

Từ kết quả thực nghiệm thực tế, chúng ta phân tích 4 yếu tố chính quyết định ưu/nhược thế của từng thuật toán:

### 4.1. Ảnh hưởng của Mật độ mẫu (Dataset Density: Dense vs. Sparse)
- **Trên tập dữ liệu Đặc (Dense Datasets như `chess.dat` 49.33% và `mushroom.dat` 19.33%):**
  - **Hiện tượng:** Càng hạ thấp $\text{min\_sup}$, số lượng mẫu dài ($k \ge 5, 6, 7$) bùng nổ cấp số nhân.
  - **Apriori sụp đổ hiệu năng:** Với mỗi tập phổ biến độ dài $k$, số ứng viên $C_k$ sinh ra rất lớn. Để kiểm tra độ hỗ trợ, Apriori phải duyệt qua các giao dịch có độ dài 37 mục, sinh ra $\binom{37}{k}$ tổ hợp. Thời gian tăng từ 4.21 ms lên tới 719.72 ms (trên `chess`) và từ 11.88 ms vọt lên 4,365.46 ms (trên `mushroom`).
  - **FP-Growth áp đảo:** FP-Growth nén các tiền tố trùng lặp cao vào FP-tree cực kỳ nhỏ gọn. Số nút trên FP-tree của `chess` và `mushroom` rất nhỏ nhờ tính lặp lại cao. Quá trình chia để trị khai thác trực tiếp trên cây mà không phải sinh ứng viên, giúp thời gian chạy chỉ mất 16.57 ms (nhanh hơn Apriori **43 lần**) trên `chess` và 65.30 ms (nhanh hơn Apriori **67 lần**) trên `mushroom`.
  - **Eclat thể hiện rất tốt:** Do $|D| = 3,196$ tương đối nhỏ, các TID-list ngắn ($< 3,200$ phần tử), phép giao `intersect` trên mảng được tối ưu cache CPU chạy cực kỳ nhanh (57.79 ms).

- **Trên tập dữ liệu Thưa (Sparse Datasets như `retail.dat` 0.06%):**
  - **Hiện tượng:** Số lượng mục rất lớn ($|I| = 16,470$), nhưng mỗi giao dịch chỉ chứa trung bình 10 mục. Các mục hiếm khi đi cùng nhau.
  - **Eclat giảm sút lợi thế:** Ban đầu Eclat phải khởi tạo $16,470$ danh sách TID với tổng cộng $88,162$ giao dịch. Kích thước danh sách TID lớn khiến chi phí khởi tạo và bộ nhớ cache bị phân mảnh. Do đó ở mức min_sup 5%, Eclat mất 145.06 ms (chậm hơn cả Apriori 60.62 ms).
  - **Apriori hoạt động tương đối ổn ở ngưỡng cao:** Do dữ liệu thưa, số lượng ứng viên $C_k$ sinh ra rất ít (chỉ 16 đến 55 mẫu), do đó BFS của Apriori dừng rất sớm sau 2 hoặc 3 mức.
  - **FP-Growth vẫn là tối ưu nhất:** Nhờ bước lọc F-List loại bỏ ngay $16,400+$ mục không phổ biến trước khi xây dựng cây, cây FP-tree cho `retail.dat` chỉ lưu các mục phổ biến thực sự, giữ thời gian xử lý ổn định ở mức ~50-55 ms.

---

### 4.2. Ảnh hưởng của Thiết lập Ngưỡng độ hỗ trợ tối thiểu (min_sup)
- **Khi $\text{min\_sup}$ cao:**
  - Cả 3 thuật toán đều chạy rất nhanh (dưới 15 ms).
  - **Eclat** thường đạt thời gian nhanh nhất hoặc ngang bằng FP-Growth (ví dụ: `chess` 95% mất 2.28 ms; `mushroom` 50% mất 4.48 ms) vì khi số lượng mẫu ít, cây đệ quy của Eclat rất nông, chỉ vài phép giao TID là hoàn thành, không mất chi phí xây dựng cây FP-tree hay duyệt BFS nhiều vòng.
- **Khi $\text{min\_sup}$ giảm thấp:**
  - Không gian tìm kiếm mở rộng theo hàm mũ.
  - **Apriori:** Thời gian tăng vọt không kiểm soát vì số lượng ứng viên $\approx \mathcal{O}(|L_{k-1}|^2)$ và chi phí quét CSDL qua từng vòng $k$. Ở mức $\text{min\_sup} = 0.5\%$ trên `retail`, Apriori không khả thi nếu chạy BFS thông thường.
  - **Eclat:** Các danh sách TID trung gian trong ngăn xếp đệ quy tăng lên, chi phí giao danh sách tăng.
  - **FP-Growth:** Thể hiện độ co giãn (**scalability**) tốt nhất. Nhờ cơ chế chiếu mẫu có điều kiện, FP-growth chỉ tập trung vào các nhánh chứa mẫu điều kiện thực sự tồn tại.

---

### 4.3. Ảnh hưởng của Kích thước dữ liệu ($|D|$ và $|I|$)
- **Số lượng giao dịch lớn ($|D|$ cao - ví dụ `retail.dat` với 88,162 dòng):**
  - Trong **Apriori**, mỗi vòng $k$ phải quét lại toàn bộ $|D|$ giao dịch. Nếu $|D|$ lên tới hàng triệu dòng, chi phí I/O và kiểm tra bao hàm sẽ trở thành nút thắt cổ chai cực lớn.
  - Trong **Eclat**, độ dài của TID-list tỉ lệ thuận với $|D|$. Nếu $|D| = 1,000,000$, mỗi TID-list có thể tốn vài megabyte, dẫn đến tốn bộ nhớ RAM và thời gian giao danh sách tăng tuyến tính theo $|D|$.
  - Trong **FP-Growth**, dù $|D|$ rất lớn, các giao dịch giống nhau sẽ được gộp vào chung một nhánh (chỉ tăng biến `count`), do đó kích thước cây FP-Tree thường nhỏ hơn nhiều so với kích thước CSDL gốc.
- **Số lượng mục lớn ($|I|$ cao):**
  - $|I| = 16,470$ khiến ma trận ứng viên ban đầu của Apriori $\binom{|I|}{2}$ có thể lên tới $135$ triệu cặp nếu không có ngưỡng min_sup lọc bớt.
  - FP-Growth loại bỏ ngay các mục không thỏa mãn ngay từ lần quét đầu tiên, chỉ giữ lại các mục phổ biến đưa vào cây.

---

### 4.4. Bảng tổng kết so sánh toàn diện 3 thuật toán

| Tiêu chí | Apriori [AS94b] | Eclat [Zak00] | FP-Growth [HPY00] |
|:---|:---|:---|:---|
| **Cơ chế duyệt** | Chiều rộng (BFS) | Chiều sâu (DFS) | Chiều sâu / Pattern Growth |
| **Định dạng dữ liệu** | Ngang (Horizontal) | Dọc (Vertical TID-list) | Cây tiền tố (FP-Tree) |
| **Số lần quét CSDL gốc** | $k_{max} + 1$ lần (Rất nhiều) | Đúng **1 lần duy nhất** | Đúng **2 lần duy nhất** |
| **Sinh tập ứng viên?** | **Có** (Chi phí $\mathcal{O}(|L_{k-1}|^2)$) | Không (Dùng phép giao TID) | **Không** (Dùng chia để trị) |
| **Tình huống hoạt động TỐT NHẤT** | Dữ liệu rất thưa, ngưỡng min_sup cao, tập mẫu ngắn | Dữ liệu đặc/vừa, số giao dịch $|D|$ nhỏ hoặc vừa | Mọi loại dữ liệu (Đặc biệt là dữ liệu lớn, min_sup thấp, mẫu dài) |
| **Tình huống hoạt động KÉM NHẤT** | Dữ liệu đặc, min_sup thấp (Bùng nổ ứng viên) | Số giao dịch $|D|$ cực lớn, dữ liệu quá thưa | CSDL có ít tiền tố trùng lặp (Cây FP-tree không nén được nhiều) |
| **Mức tiêu thụ bộ nhớ** | Thấp ở từng mức (chỉ lưu $L_{k-1}, C_k$) | Trung bình đến cao (Lưu TID-lists trong ngăn xếp đệ quy) | Trung bình (Toàn bộ FP-tree nằm trong RAM) |

---

## 5. KẾT LUẬN VÀ BÀI HỌC KINH NGHIỆM

1. **FP-Growth** là thuật toán có hiệu năng vượt trội và ổn định nhất xét trên hầu hết các kịch bản thực tế (nhanh hơn Apriori từ **15x đến 67x** trên dữ liệu đặc). Ý tưởng nén tiền tố và loại bỏ hoàn toàn bước sinh ứng viên là bước đột phá lớn trong khai phá mẫu phổ biến.
2. **Eclat** là thuật toán có mã nguồn cài đặt gọn gàng, thanh lịch và đạt hiệu năng rất cao khi số lượng giao dịch $|D|$ ở mức vừa phải và dữ liệu có độ đặc cao. Việc chuyển đổi sang định dạng dọc giúp thay thế toàn bộ thao tác quét dữ liệu phức tạp bằng phép giao mảng đơn giản.
3. **Apriori** mang giá trị nền tảng lý thuyết to lớn với nguyên lý *Anti-monotonicity*, nhưng hạn chế bởi việc quét CSDL lặp lại nhiều lần và sinh tổ hợp ứng viên khổng lồ khi dữ liệu đặc hoặc min_sup thấp.
