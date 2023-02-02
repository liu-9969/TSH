Author=liuxiangle
Version=0.0.1

CXX := g++

FLAGS_CC := -g -std=c++11 -w -DELPP_NO_DEFAULT_LOG_FILE
LIB_CC := -lpthread

FLAGS_AGENT := -g -std=c++11 -w -static -fpic 
LIB_AGENT := -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lutil

RM := rm -rf



# LIB_CC_DIRS = $(shell find core/lib/cc -maxdepth 3 -type d)
# LIB_CC_SRC += $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
LIB_CCNET_SRC := ${wildcard core/lib/cc/net/*.cpp}
LIB_CCNET_OBJ := ${patsubst %.cpp,%.o,$(LIB_CCNET_SRC)}
LIB_CC_SRC := ${wildcard core/lib/cc/*.cpp}
LIB_CC_OBJ := ${patsubst %.cpp,%.o,$(LIB_CC_SRC)}
TSH_CC_SRC := ${wildcard core/tsh/cc/*.cpp}
TSH_CC_OBJ := ${patsubst %.cpp,%.o,$(TSH_CC_SRC)}
LIB_CC_CCNET := ${wildcard core/lib/cc/net/*.h}
LIB_CC_H := ${wildcard core/lib/cc/*.h}

LIB_AGENTNET_SRC := ${wildcard core/lib/agent/net/*.cpp}
LIB_AGENTNET_OBJ := ${patsubst %.cpp,%.o,$(LIB_AGENTNET_SRC)}
LIB_AGENT_SRC := ${wildcard core/lib/agent/*.cpp}
LIB_AGENT_OBJ := ${patsubst %.cpp,%.o,$(LIB_AGENT_SRC)}
TSH_AGENT_SRC := ${wildcard core/tsh/agent/*.cpp}
TSH_AGENT_OBJ := ${patsubst %.cpp,%.o,$(TSH_AGENT_SRC)}
LIB_AGENT_H := ${wildcard core/lib/agent/*.h}
LIB_AGENTNET_H := ${wildcard core/lib/agent/net/*.h}

LOG_SRC=Log/easylogging++.cc
LOG_OBJ=${patsubst %.cpp,%.o,$(LOG_SRC)}
LOG_H=Log/easylogging++.h
	


all: info input1 CC input2 agent clean done

info:
	$(info )
	$(info Author:$(Author))
	$(info Version:$(Version))

input1:
	$(info )
	@echo "[compiling CC...]"
	$(info )


CC: ${LIB_CC_OBJ} ${TSH_CC_OBJ} ${LIB_CCNET_OBJ} ${LOG_OBJ}
	${CXX} ${FLAGS_CC} -o $@ $^ ${LIB_CC}   
${LIB_CC_OBJ} ${TSH_CC_OBJ} ${LIB_CCNET_OBJ} ${LOG_OBJ}: %.o: %.cpp ${LIB_CC_H} ${LIB_CC_CCNET} ${LOG_H} core/setting.h
	${CXX} ${FLAGS_CC} -o $@ -c $< 


input2:
	$(info )
	@echo "[compiling agent...]"
	$(info )


agent: ${LIB_AGENT_OBJ} ${TSH_AGENT_OBJ} ${LIB_AGENTNET_OBJ} 
	${CXX} ${FLAGS_AGENT} -o $@ $^ ${LIB_AGENT}
${LIB_AGENT_OBJ} ${TSH_AGENT_OBJ} ${LIB_AGENTNET_OBJ}: %.o: %.cpp ${LIB_AGENT_H} ${LIB_AGENTNET_H} core/setting.h
	${CXX} ${FLAGS_AGENT} -o $@ -c $< 

clean:
	@echo
	${RM} ${LIB_AGENT_OBJ}  ${LIB_CC_OBJ} ${TSH_CC_OBJ} ${LIB_CCNET_OBJ} ${TSH_AGENT_OBJ} ${LIB_AGENTNET_OBJ}

done:
	# strip agent
	@echo
	@echo "[Build SUCCESS]"

.PHONY: clean
cleanall:
	${RM} CC agent


