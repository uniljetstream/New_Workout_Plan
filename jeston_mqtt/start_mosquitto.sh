#!/bin/bash

echo "🟢 Mosquitto 시작 스크립트"
echo "========================================"

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 현재 Mosquitto 상태 확인
echo ""
echo "📊 현재 Mosquitto 상태 확인..."
if sudo systemctl is-active mosquitto >/dev/null 2>&1; then
    echo -e "${YELLOW}⚠️  Mosquitto가 이미 실행 중입니다!${NC}"
    sudo systemctl status mosquitto --no-pager | head -n 5
    echo ""
    read -p "재시작하시겠습니까? (y/n): " restart_choice
    if [ "$restart_choice" = "y" ] || [ "$restart_choice" = "Y" ]; then
        echo -e "${YELLOW}🔄 Mosquitto 재시작 중...${NC}"
        sudo systemctl restart mosquitto
        sleep 2
    else
        echo -e "${GREEN}✅ 기존 Mosquitto를 유지합니다.${NC}"
        exit 0
    fi
else
    echo -e "${BLUE}ℹ️  Mosquitto가 실행 중이지 않습니다.${NC}"
fi

# 포트 1883 사용 여부 확인
echo ""
echo "🔍 포트 1883 확인 중..."
if sudo lsof -i :1883 >/dev/null 2>&1; then
    echo -e "${RED}❌ 포트 1883이 다른 프로세스에서 사용 중입니다:${NC}"
    sudo lsof -i :1883
    echo ""
    read -p "해당 프로세스를 종료하고 Mosquitto를 시작하시겠습니까? (y/n): " kill_choice
    if [ "$kill_choice" = "y" ] || [ "$kill_choice" = "Y" ]; then
        sudo kill -9 $(sudo lsof -t -i:1883) 2>/dev/null
        echo -e "${GREEN}✅ 포트 1883을 정리했습니다.${NC}"
    else
        echo -e "${RED}❌ Mosquitto를 시작할 수 없습니다. 포트가 사용 중입니다.${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✅ 포트 1883이 사용 가능합니다.${NC}"
fi

# Mosquitto 설정 파일 확인
echo ""
echo "⚙️  설정 파일 확인..."
if [ ! -f /etc/mosquitto/mosquitto.conf ]; then
    echo -e "${RED}❌ mosquitto.conf 파일이 없습니다!${NC}"
    echo "설정 파일을 생성합니다..."
    
    sudo tee /etc/mosquitto/mosquitto.conf > /dev/null <<'EOF'
# Mosquitto 기본 설정
pid_file /var/run/mosquitto/mosquitto.pid
persistence true
persistence_location /var/lib/mosquitto/
log_dest file /var/log/mosquitto/mosquitto.log
log_type all
listener 1883
protocol mqtt
allow_anonymous true
EOF
    echo -e "${GREEN}✅ 기본 설정 파일을 생성했습니다.${NC}"
else
    echo -e "${GREEN}✅ 설정 파일이 존재합니다.${NC}"
fi

# 필요한 디렉토리 생성
echo ""
echo "📁 필요한 디렉토리 확인 및 생성..."
sudo mkdir -p /var/run/mosquitto
sudo mkdir -p /var/log/mosquitto
sudo mkdir -p /var/lib/mosquitto
sudo chown -R mosquitto:mosquitto /var/run/mosquitto 2>/dev/null
sudo chown -R mosquitto:mosquitto /var/log/mosquitto 2>/dev/null
sudo chown -R mosquitto:mosquitto /var/lib/mosquitto 2>/dev/null
echo -e "${GREEN}✅ 디렉토리 준비 완료${NC}"

# Mosquitto 시작
echo ""
echo "========================================"
echo -e "${GREEN}🚀 Mosquitto 시작 중...${NC}"
sudo systemctl daemon-reload
sudo systemctl start mosquitto

# 시작 대기
sleep 2

# 시작 확인
echo ""
if sudo systemctl is-active mosquitto >/dev/null 2>&1; then
    echo -e "${GREEN}✅ Mosquitto가 성공적으로 시작되었습니다!${NC}"
    echo ""
    echo "📋 서비스 상태:"
    sudo systemctl status mosquitto --no-pager | head -n 10
    
    echo ""
    echo "🌐 네트워크 정보:"
    echo -e "  • 브로커 주소: ${GREEN}localhost${NC} 또는 ${GREEN}$(hostname -I | awk '{print $1}')${NC}"
    echo -e "  • 포트: ${GREEN}1883${NC}"
    echo -e "  • 프로토콜: ${GREEN}MQTT${NC}"
    
    echo ""
    echo "📊 포트 상태:"
    sudo netstat -tlnp | grep 1883 || sudo ss -tlnp | grep 1883
    
    echo ""
    echo "📝 최근 로그:"
    sudo tail -n 5 /var/log/mosquitto/mosquitto.log 2>/dev/null || echo "로그 파일이 아직 생성되지 않았습니다."
    
    # 자동 시작 설정
    echo ""
    read -p "부팅 시 자동으로 시작하도록 설정하시겠습니까? (y/n): " autostart_choice
    if [ "$autostart_choice" = "y" ] || [ "$autostart_choice" = "Y" ]; then
        sudo systemctl enable mosquitto
        echo -e "${GREEN}✅ 부팅 시 자동 시작이 설정되었습니다.${NC}"
    fi
else
    echo -e "${RED}❌ Mosquitto 시작 실패!${NC}"
    echo ""
    echo "에러 로그:"
    sudo journalctl -u mosquitto -n 20 --no-pager
    echo ""
    echo "디버그를 위해 수동으로 실행해보세요:"
    echo "  sudo mosquitto -c /etc/mosquitto/mosquitto.conf -v"
    exit 1
fi

echo ""
echo "========================================"
echo -e "${GREEN}✅ Mosquitto MQTT 브로커가 실행 중입니다!${NC}"
echo ""
echo "🧪 테스트 명령어:"
echo "  • 구독 테스트: mosquitto_sub -t test/# -v"
echo "  • 발행 테스트: mosquitto_pub -t test/topic -m 'Hello MQTT'"
echo "  • 수신 프로그램: python3 receiver.py"
echo "  • 발송 프로그램: python3 sender.py"
echo ""
echo "🔧 관리 명령어:"
echo "  • 상태 확인: sudo systemctl status mosquitto"
echo "  • 로그 확인: sudo tail -f /var/log/mosquitto/mosquitto.log"
echo "  • 서비스 중지: sudo systemctl stop mosquitto"
echo "  • 서비스 재시작: sudo systemctl restart mosquitto"