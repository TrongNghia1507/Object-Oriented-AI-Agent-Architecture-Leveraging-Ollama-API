#!/bin/bash

echo "================ 1. CÀI ĐẶT THƯ VIỆN ================"
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libsqlite3-dev
sudo apt install -y g++-14 gcc-14

echo "================ 2. THIẾT LẬP SEARXNG ==============="
# Tắt container cũ nếu có (bỏ qua thông báo lỗi nếu không có container)
docker stop $(docker ps -q --filter ancestor=searxng/searxng) 2>/dev/null

mkdir -p searxng_config
cat << 'EOF' > searxng_config/settings.yml
use_default_settings: true
general:
  debug: false
  instance_name: "Local SearXNG"
search:
  safe_search: 0
  autocomplete: ""
  formats:
    - html
    - json
server:
  port: 8080
  bind_address: "0.0.0.0"
  secret_key: "agent_ai_secret_key_change_me"
EOF

# Chạy Docker SearXNG
docker run --rm -d \
  -p 8080:8080 \
  -v $(pwd)/searxng_config/settings.yml:/etc/searxng/settings.yml:ro \
  searxng/searxng

echo "================ 3. THIẾT LẬP OLLAMA ================"
# Kiểm tra nếu chưa có ollama thì mới tải về
if ! command -v ollama &> /dev/null; then
    curl -fsSL https://ollama.com/install.sh | sh
fi

# Chạy ollama ngầm để script tiếp tục chạy
ollama serve > /dev/null 2>&1 &

# Chờ 5 giây để server Ollama khởi động lên hoàn toàn
sleep 5

# Tải model embedding
ollama pull nomic-embed-text


echo ""
echo "================ 4. KIỂM TRA TRẠNG THÁI (VERIFICATION) ================"
ALL_PASSED=true

# 4.1 Kiểm tra SearXNG
if docker ps -q --filter ancestor=searxng/searxng | grep -q .; then
    echo "[OK] Docker SearXNG đang chạy."
else
    echo "[LỖI] Docker SearXNG không chạy!"
    ALL_PASSED=false
fi

# 4.2 Kiểm tra Ollama Server
if curl -s http://localhost:11434/ | grep -q "Ollama is running"; then
    echo "[OK] Ollama server đang hoạt động ở cổng 11434."
else
    echo "[LỖI] Ollama server không phản hồi!"
    ALL_PASSED=false
fi

# 4.3 Kiểm tra Model Embedding
if ollama list | grep -q "nomic-embed-text"; then
    echo "[OK] Model 'nomic-embed-text' đã được tải thành công."
else
    echo "[LỖI] Không tìm thấy model 'nomic-embed-text'!"
    ALL_PASSED=false
fi

echo "====================== KẾT QUẢ ======================"
if [ "$ALL_PASSED" = true ]; then
    echo "Mọi thành phần đều đã sẵn sàng. Bạn có thể chạy Agent ngay bây giờ."
else
    echo "Có thành phần bị lỗi. Hãy xem lại các dòng [LỖI] ở trên."
fi