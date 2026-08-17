#!/bin/bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libsqlite3-dev
sudo apt install -y g++-14 gcc-14

docker stop $(docker ps -q --filter ancestor=searxng/searxng)
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
docker run --rm -d \
  -p 8080:8080 \
  -v $(pwd)/searxng_config/settings.yml:/etc/searxng/settings.yml:ro \
  searxng/searxng

curl -fsSL https://ollama.com/install.sh | sh
ollama serve
ollama pull nomic-embed-text
