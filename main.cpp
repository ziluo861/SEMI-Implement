#include <cstring>
#include <iostream>
#include "FiniteStateMachine/State/State.hpp"
#include "FiniteStateMachine/FSM.hpp"
#include "FiniteStateMachine/State/StateChangeHandler.hpp"
#include "Requirements/RequirementMonitor.hpp"
#include <SECS/SECSParser.hpp>
#include <VarRef/SourceVarRef.hpp>
#include <vector>
#include <memory>
#include <cassert>

enum class StateId {
    IDLE,
    WORKING,
    PAUSED,
    ERROR,
    SHUTDOWN,
    // 层次状态
    MACHINE_ROOT,
    MACHINE_ON,
    MACHINE_OFF,
    MOTOR_RUNNING,
    MOTOR_STOPPED
};

namespace std {
    template<>
    struct hash<StateId> {
        size_t operator()(const StateId& id) const {
            return static_cast<size_t>(id);
        }
    };
}

class PulseRequirement : public RequirementMonitor {
protected:
    void on_start() override {}
    void on_stop() override  {}
public:
    void pulse() { set_fulfilled(true); set_fulfilled(false); }
};

class TemperatureRequirement : public RequirementMonitor {
private:
    bool temperature_ok_ = false;
    
protected:
    void on_start() override {
        std::cout << "开始监控温度...\n";
        // 模拟温度检测
    }
    
    void on_stop() override {
        std::cout << "停止监控温度\n";
    }
    
public:
    void set_temperature_status(bool ok) {
        temperature_ok_ = ok;
        set_fulfilled(ok);
    }
};

// 时间需求监控器（模拟定时器）
class TimerRequirement : public RequirementMonitor {
private:
    int timer_count_ = 0;
    int target_count_;
    
protected:
    void on_start() override {
        std::cout << "启动定时器，目标: " << target_count_ << "秒\n";
        timer_count_ = 0;
    }
    
    void on_stop() override {
        std::cout << "停止定时器\n";
        timer_count_ = 0;
    }
    
public:
    explicit TimerRequirement(int seconds) : target_count_(seconds) {}
    
    void tick() {
        if (!monitoring()) return;
        timer_count_++;
        std::cout << "定时器: " << timer_count_ << "/" << target_count_ << "\n";
        if (timer_count_ >= target_count_) {
            set_fulfilled(true);
        }
    }
    
    void reset() {
        timer_count_ = 0;
        set_fulfilled(false);
    }
};

// === 测试1：基本线性状态机 ===
void test_basic_linear_fsm() {
    std::cout << "\n=== 测试1: 基本线性状态机 ===\n";

    // 1) 三个叶状态的进/出处理器
    auto idle_handler = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "进入空闲状态\n"; },
        [](State<StateId>&){ std::cout << "退出空闲状态\n"; }
    );
    auto working_handler = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "进入工作状态\n"; },
        [](State<StateId>&){ std::cout << "退出工作状态\n"; }
    );
    auto error_handler = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "进入错误状态\n"; },
        [](State<StateId>&){ std::cout << "退出错误状态\n"; }
    );

    std::vector<std::unique_ptr<State<StateId>>> states;
    states.push_back(std::make_unique<State<StateId>>(StateId::IDLE,    std::move(idle_handler)));
    states.push_back(std::make_unique<State<StateId>>(StateId::WORKING, std::move(working_handler)));
    states.push_back(std::make_unique<State<StateId>>(StateId::ERROR,   std::move(error_handler)));

    auto fsm_handler = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "FSM启动\n"; },
        [](State<StateId>&){ std::cout << "FSM停止\n"; }
    );

    FSM<StateId> fsm(StateId::MACHINE_ROOT, std::move(fsm_handler), std::move(states));

    // 订阅状态变化（打印 enum 的整数值方便核对）
    auto sub = fsm.subscribe([](const StateId& from, const StateId& to){
        std::cout << "状态转换: " << static_cast<int>(from) << " -> " << static_cast<int>(to) << "\n";
    });

    // 需求
    auto temp_ok  = std::make_unique<TemperatureRequirement>();
    auto timer_3s = std::make_unique<TimerRequirement>(3);

    // 2) 必备：根入口 -> IDLE（初始子状态）
    fsm.AppendTransition(StateId::MACHINE_ROOT, StateId::IDLE, nullptr, nullptr, false);

    // 3) 业务转移
    fsm.AppendTransition(StateId::IDLE,    StateId::WORKING, nullptr,
        [](const StateId&, const StateId&){ std::cout << "执行转换: 开始工作\n"; },
        false);

    fsm.AppendTransition(StateId::WORKING, StateId::ERROR,   temp_ok.get(),
        [](const StateId&, const StateId&){ std::cout << "执行转换: 温度异常，进入错误状态\n"; },
        false);

    fsm.AppendTransition(StateId::ERROR,   StateId::IDLE,    timer_3s.get(),
        [](const StateId&, const StateId&){ std::cout << "执行转换: 错误恢复，返回空闲\n"; },
        false);

    // 4) 启动与驱动
    std::cout << "\n--- 启动FSM ---\n";
    fsm.Start();
    std::cout << "当前状态: " << static_cast<int>(fsm.CurrentState()->Index()) << "\n";

    std::cout << "\n--- 模拟温度达标(触发 WORKING->ERROR) ---\n";
    temp_ok->set_temperature_status(true);

    std::cout << "\n--- 模拟定时器(触发 ERROR->IDLE) ---\n";
    for (int i = 0; i < 3; ++i) timer_3s->tick();

    std::cout << "\n--- 停止FSM ---\n";
    fsm.Stop();
}


// === 测试2：层次状态机 ===
void test_hierarchical_fsm() {
    std::cout << "\n=== 测试2: 层次状态机 ===\n";

    // 子状态（电机）
    auto motor_run_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "电机开始运行\n"; },
        [](State<StateId>&){ std::cout << "电机停止运行\n"; }
    );
    auto motor_stop_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "电机处于停止状态\n"; },
        [](State<StateId>&){ std::cout << "电机退出停止状态\n"; }
    );
    std::vector<std::unique_ptr<State<StateId>>> motor_states;
    motor_states.push_back(std::make_unique<State<StateId>>(StateId::MOTOR_STOPPED, std::move(motor_stop_h)));
    motor_states.push_back(std::make_unique<State<StateId>>(StateId::MOTOR_RUNNING, std::move(motor_run_h)));

    // 复合状态：ON（包含电机状态）
    auto on_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "机器开启\n"; },
        [](State<StateId>&){ std::cout << "机器关闭\n"; }
    );
    auto st_on = std::make_unique<State<StateId>>(StateId::MACHINE_ON, std::move(on_h), std::move(motor_states));

    // 简单状态：OFF
    auto off_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "机器关闭状态\n"; },
        [](State<StateId>&){ std::cout << "退出机器关闭状态\n"; }
    );
    auto st_off = std::make_unique<State<StateId>>(StateId::MACHINE_OFF, std::move(off_h));

    // 根
    auto root_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "层次状态机启动\n"; },
        [](State<StateId>&){ std::cout << "层次状态机停止\n"; }
    );

    std::vector<std::unique_ptr<State<StateId>>> root_states;
    root_states.push_back(std::move(st_off));
    root_states.push_back(std::move(st_on));

    FSM<StateId> hsm(StateId::MACHINE_ROOT, std::move(root_h), std::move(root_states));

    auto sub = hsm.subscribe([](const StateId& from, const StateId& to){
        std::cout << "层次状态转换: " << static_cast<int>(from) << " -> " << static_cast<int>(to) << "\n";
    });

    // 需求
    PulseRequirement to_on, to_off, to_run, to_stop;

    // 初始：根 -> OFF
    hsm.AppendTransition(StateId::MACHINE_ROOT, StateId::MACHINE_OFF, nullptr,
        [](const StateId&, const StateId&){ std::cout << "从根进入默认子状态: MACHINE_OFF\n"; }, false);

    // 进入 ON 时默认进其子状态 MOTOR_STOPPED
    hsm.AppendTransition(StateId::MACHINE_ON, StateId::MOTOR_STOPPED, nullptr, nullptr, false);

    // OFF <-> ON
    hsm.AppendTransition(StateId::MACHINE_OFF, StateId::MACHINE_ON, &to_on,  [](...) {}, false);
    hsm.AppendTransition(StateId::MACHINE_ON,  StateId::MACHINE_OFF, &to_off, [](...) {}, false);

    // ON 内部：STOPPED <-> RUNNING
    hsm.AppendTransition(StateId::MOTOR_STOPPED, StateId::MOTOR_RUNNING, &to_run,  [](...) {}, false);
    hsm.AppendTransition(StateId::MOTOR_RUNNING, StateId::MOTOR_STOPPED, &to_stop, [](...) {}, false);

    std::cout << "\n--- 启动层次状态机 ---\n";
    hsm.Start();

    std::cout << "\n--- ON 脉冲（OFF->ON，并默认进入 MOTOR_STOPPED）---\n";
    to_on.pulse();

    std::cout << "\n--- RUN 脉冲（MOTOR_STOPPED->MOTOR_RUNNING）---\n";
    to_run.pulse();

    std::cout << "\n--- STOP 脉冲（MOTOR_RUNNING->MOTOR_STOPPED）---\n";
    to_stop.pulse();

    std::cout << "\n--- OFF 脉冲（ON->OFF）---\n";
    to_off.pulse();

    std::cout << "\n--- 停止层次状态机 ---\n";
    hsm.Stop();
}


// 测试3: 自转换和历史状态
void test_self_transition_and_history() {
    std::cout << "\n=== 测试3: 自转换和历史状态 ===\n";

    // 子图：电机
    auto run_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "电机开始运行\n"; },
        [](State<StateId>&){ std::cout << "电机停止运行\n"; }
    );
    auto stop_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "电机处于停止状态\n"; },
        [](State<StateId>&){ std::cout << "电机退出停止状态\n"; }
    );
    std::vector<std::unique_ptr<State<StateId>>> motor;
    motor.push_back(std::make_unique<State<StateId>>(StateId::MOTOR_STOPPED, std::move(stop_h)));
    motor.push_back(std::make_unique<State<StateId>>(StateId::MOTOR_RUNNING, std::move(run_h)));

    auto on_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "进入 ON 复合状态\n"; },
        [](State<StateId>&){ std::cout << "退出 ON 复合状态\n"; }
    );
    auto st_on = std::make_unique<State<StateId>>(StateId::MACHINE_ON, std::move(on_h), std::move(motor));

    std::vector<std::unique_ptr<State<StateId>>> root_states;
    root_states.push_back(std::move(st_on));

    auto root_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "自转换示例 FSM 启动\n"; },
        [](State<StateId>&){ std::cout << "自转换示例 FSM 停止\n"; }
    );
    FSM<StateId> fsm(StateId::MACHINE_ROOT, std::move(root_h), std::move(root_states));

    auto sub = fsm.subscribe([](const StateId& from, const StateId& to){
        std::cout << "状态转换: " << static_cast<int>(from) << " -> " << static_cast<int>(to) << "\n";
    });

    PulseRequirement to_run;      // STOPPED -> RUNNING
    TimerRequirement timer2(2);   // ON -> ON (enterHistory)

    // ROOT -> ON
    fsm.AppendTransition(StateId::MACHINE_ROOT, StateId::MACHINE_ON, nullptr, nullptr, false);
    // ON 默认落到 STOPPED
    fsm.AppendTransition(StateId::MACHINE_ON, StateId::MOTOR_STOPPED, nullptr, nullptr, false);
    // STOPPED -> RUNNING
    fsm.AppendTransition(StateId::MOTOR_STOPPED, StateId::MOTOR_RUNNING, &to_run, nullptr, false);
    // ON 的自转移（enterHistory=true）
    fsm.AppendTransition(StateId::MACHINE_ON, StateId::MACHINE_ON, &timer2,
        [](auto,auto){ std::cout << "执行自转换: ON 重启（恢复历史子状态）\n"; }, true);

    std::cout << "\n--- 启动 ---\n";
    fsm.Start();                 // ROOT->ON，ON->(Entrance)->STOPPED

    std::cout << "\n--- 让电机先 RUN（STOPPED->RUNNING）---\n";
    to_run.pulse();              // STOPPED->RUNNING（历史叶子=RUNNING）

    std::cout << "\n--- 触发定时器两次，触发 ON 的自转移（应恢复到 RUNNING）---\n";
    timer2.tick();               // 1/2
    timer2.tick();               // 2/2 触发 ON 自转移：退出 ON -> 执行动作 -> 直接进入“历史叶子 RUNNING”

    std::cout << "\n--- 停止 ---\n";
    fsm.Stop();
}



// 测试4: 错误处理和边界条件
void test_error_conditions() {
    std::cout << "\n=== 测试4: 错误处理测试 ===\n";
    
    try {
        // 测试重复状态索引
        std::vector<std::unique_ptr<State<StateId>>> duplicate_states;
        duplicate_states.push_back(std::make_unique<State<StateId>>(StateId::IDLE));
        duplicate_states.push_back(std::make_unique<State<StateId>>(StateId::IDLE)); // 重复索引
        
        std::cout << "尝试创建重复索引的FSM...\n";
        try {
            FSM<StateId> fsm(StateId::IDLE, nullptr, std::move(duplicate_states));
            std::cout << "错误：应该抛出异常但没有\n";
        } catch (const std::runtime_error& e) {
            std::cout << "正确捕获异常: " << e.what() << "\n";
        }
        
        // 测试不存在的状态转换
        std::vector<std::unique_ptr<State<StateId>>> valid_states;
        valid_states.push_back(std::make_unique<State<StateId>>(StateId::IDLE));
        
        FSM<StateId> fsm(StateId::IDLE, nullptr, std::move(valid_states));
        
        std::cout << "尝试添加不存在状态的转换...\n";
        try {
            fsm.AppendTransition(StateId::IDLE, StateId::WORKING, nullptr, nullptr, false);
            std::cout << "错误：应该抛出异常但没有\n";
        } catch (const std::runtime_error& e) {
            std::cout << "正确捕获异常: " << e.what() << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "错误处理测试异常: " << e.what() << "\n";
    }
}

// === 测试4：跨分支(LCA) + 历史回放 ===
// 结构：ROOT 下有 { OFF, ON{STOPPED/RUNNING}, ERROR } 三支
// 场景：OFF->ON(默认 STOPPED) -> RUNNING -> (跨分支)RUNNING->ERROR -> ERROR->OFF
//      然后 OFF->ON (enterHistory=true) 应直接回到 RUNNING（历史叶）
void test_cross_branch_and_history_reentry() {
    std::cout << "\n=== 测试4: 跨分支(LCA) + 历史回放 ===\n";

    // 子图（电机）
    auto run_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "电机开始运行\n"; },
        [](State<StateId>&){ std::cout << "电机停止运行\n"; }
    );
    auto stop_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "电机处于停止状态\n"; },
        [](State<StateId>&){ std::cout << "电机退出停止状态\n"; }
    );
    std::vector<std::unique_ptr<State<StateId>>> motor;
    motor.push_back(std::make_unique<State<StateId>>(StateId::MOTOR_STOPPED, std::move(stop_h)));
    motor.push_back(std::make_unique<State<StateId>>(StateId::MOTOR_RUNNING, std::move(run_h)));

    // 复合 ON
    auto on_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "机器开启\n"; },
        [](State<StateId>&){ std::cout << "机器关闭\n"; }
    );
    auto st_on = std::make_unique<State<StateId>>(StateId::MACHINE_ON, std::move(on_h), std::move(motor));

    // 简单 OFF
    auto off_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "机器关闭状态\n"; },
        [](State<StateId>&){ std::cout << "退出机器关闭状态\n"; }
    );
    auto st_off = std::make_unique<State<StateId>>(StateId::MACHINE_OFF, std::move(off_h));

    // 简单 ERROR
    auto err_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "进入错误状态\n"; },
        [](State<StateId>&){ std::cout << "退出错误状态\n"; }
    );
    auto st_err = std::make_unique<State<StateId>>(StateId::ERROR, std::move(err_h));

    // 根
    auto root_h = std::make_unique<StateChangeHandler<StateId>>(
        [](State<StateId>&){ std::cout << "跨分支 FSM 启动\n"; },
        [](State<StateId>&){ std::cout << "跨分支 FSM 停止\n"; }
    );

    std::vector<std::unique_ptr<State<StateId>>> root_states;
    root_states.push_back(std::move(st_off));
    root_states.push_back(std::move(st_on));
    root_states.push_back(std::move(st_err));

    FSM<StateId> fsm(StateId::MACHINE_ROOT, std::move(root_h), std::move(root_states));

    auto sub = fsm.subscribe([](const StateId& from, const StateId& to){
        std::cout << "状态转换: " << static_cast<int>(from) << " -> " << static_cast<int>(to) << "\n";
    });

    // 触发器
    PulseRequirement to_on, to_off, to_run, to_err, back_on;

    // ROOT->OFF
    fsm.AppendTransition(StateId::MACHINE_ROOT, StateId::MACHINE_OFF, nullptr,
        [](auto,auto){ std::cout << "从根进入默认子状态: OFF\n"; }, false);

    // 进入 ON 默认落 STOPPED
    fsm.AppendTransition(StateId::MACHINE_ON, StateId::MOTOR_STOPPED, nullptr, nullptr, false);

    // OFF <-> ON（注意 OFF->ON 这个设置 enterHistory=true 只在“回到 ON”那次才生效，我们复用另一个触发器 back_on）
    fsm.AppendTransition(StateId::MACHINE_OFF, StateId::MACHINE_ON, &to_on, nullptr, false);             // 第一次进入：false，落 STOPPED
    fsm.AppendTransition(StateId::MACHINE_OFF, StateId::MACHINE_ON, &back_on, nullptr, true);            // 回放进入：true，落历史叶

    // STOPPED <-> RUNNING
    fsm.AppendTransition(StateId::MOTOR_STOPPED, StateId::MOTOR_RUNNING, &to_run, nullptr, false);
    fsm.AppendTransition(StateId::MOTOR_RUNNING, StateId::MOTOR_STOPPED, &to_off, nullptr, false); // 备用，不一定用

    // 跨分支：RUNNING -> ERROR（LCA=ROOT）
    fsm.AppendTransition(StateId::MOTOR_RUNNING, StateId::ERROR, &to_err, nullptr, false);

    // ERROR -> OFF（回到 OFF）
    fsm.AppendTransition(StateId::ERROR, StateId::MACHINE_OFF, &to_off, nullptr, false);

    std::cout << "\n--- 启动跨分支FSM ---\n";
    fsm.Start();

    std::cout << "\n--- OFF->ON ---\n";
    to_on.pulse();       // OFF->ON->STOPPED

    std::cout << "\n--- STOPPED->RUNNING ---\n";
    to_run.pulse();      // STOPPED->RUNNING（记录历史叶为 RUNNING）

    std::cout << "\n--- RUNNING->ERROR（跨分支） ---\n";
    to_err.pulse();      // RUNNING -> ERROR

    std::cout << "\n--- ERROR->OFF ---\n";
    to_off.pulse();      // ERROR -> OFF

    std::cout << "\n--- OFF->ON (enterHistory=true，直接回到 RUNNING) ---\n";
    back_on.pulse();     // OFF->ON（enterHistory=true） -> RUNNING

    std::cout << "\n--- 停止跨分支FSM ---\n";
    fsm.Stop();
}


// === 测试5：运行时追加转移 & 非同树转移 的异常 ===
void test_runtime_append_and_lca_error() {
    std::cout << "\n=== 测试5: 运行时追加转移 & 非同树异常 ===\n";

    // 简单 FSM：ROOT{IDLE, WORKING}
    auto idle_h = std::make_unique<StateChangeHandler<StateId>>(
        [](auto&){ std::cout << "进入IDLE\n"; }, [](auto&){ std::cout << "退出IDLE\n"; }
    );
    auto work_h = std::make_unique<StateChangeHandler<StateId>>(
        [](auto&){ std::cout << "进入WORKING\n"; }, [](auto&){ std::cout << "退出WORKING\n"; }
    );
    std::vector<std::unique_ptr<State<StateId>>> states;
    states.push_back(std::make_unique<State<StateId>>(StateId::IDLE, std::move(idle_h)));
    states.push_back(std::make_unique<State<StateId>>(StateId::WORKING, std::move(work_h)));

    FSM<StateId> fsm(StateId::MACHINE_ROOT, nullptr, std::move(states));
    auto sub = fsm.subscribe([](const StateId& f, const StateId& t){
        std::cout << "状态转换: " << static_cast<int>(f) << " -> " << static_cast<int>(t) << "\n";
    });

    // ROOT->IDLE
    fsm.AppendTransition(StateId::MACHINE_ROOT, StateId::IDLE, nullptr, nullptr, false);

    std::cout << "\n--- 启动 ---\n";
    fsm.Start();

    // 运行中试图追加转移（应抛异常）
    std::cout << "\n--- 运行时追加转移（预期抛异常） ---\n";
    try {
        fsm.AppendTransition(StateId::IDLE, StateId::WORKING, nullptr, nullptr, false);
        std::cout << "错误：未抛异常\n";
    } catch (const std::exception& e) {
        std::cout << "正确捕获异常: " << e.what() << "\n";
    }

    std::cout << "\n--- 停止 ---\n";
    fsm.Stop();

    // 非同树 LCA 异常：造两个独立 FSM，再把它们的节点混用（这里只能模拟：用一个 FSM，但构造“孤儿”状态）
    std::cout << "\n--- 非同树转移（预期抛异常） ---\n";
    try {
        // 构造一个“孤儿”状态，手动拿指针是不允许的；这里用现有 API 是没法插入到 all_states_ 的，
        // 直接调用 AppendTransition 只允许用枚举索引查找；所以我们用“不同树”的等价测试：
        // 在 *另一个* FSM 上建立 WORKING，尝试在本 FSM 里从 IDLE -> WORKING（WORKING 不在这棵树里），
        // 这会触发 Not found destination。
        std::vector<std::unique_ptr<State<StateId>>> only_idle;
        only_idle.push_back(std::make_unique<State<StateId>>(StateId::IDLE));
        FSM<StateId> fsm2(StateId::MACHINE_ROOT, nullptr, std::move(only_idle));
        // fsm2 只有 IDLE，没有 WORKING
        fsm2.AppendTransition(StateId::MACHINE_ROOT, StateId::IDLE, nullptr, nullptr, false);
        // 尝试添加 IDLE->WORKING（目的不存在），会抛 "Not found destination"
        try {
            fsm2.AppendTransition(StateId::IDLE, StateId::WORKING, nullptr, nullptr, false);
            std::cout << "错误：未抛异常\n";
        } catch (const std::exception& e) {
            std::cout << "正确捕获异常: " << e.what() << "\n";
        }
    } catch (...) {
        std::cout << "（说明：非同树场景用现有 API 等价成“目的不存在”来验证）\n";
    }
}


// === 测试6：级联转移（进入即满足，连跳多步） ===
// 结构：ROOT{ IDLE ->(auto)-> WORKING ->(auto)-> ERROR }
// 两条转移的 Requirement 在 on_start() 里立即满足，观察自动连跳
class AutoRequirement : public RequirementMonitor {
protected:
    void on_start() override {
        std::cout << "自动条件满足\n";
        set_fulfilled(true);
    }
    void on_stop() override {}
};

void test_cascade_auto_transitions() {
    std::cout << "\n=== 测试6: 级联转移（进入即满足，连跳多步） ===\n";

    auto idle_h = std::make_unique<StateChangeHandler<StateId>>(
        [](auto&){ std::cout << "进入IDLE\n"; }, [](auto&){ std::cout << "退出IDLE\n"; }
    );
    auto work_h = std::make_unique<StateChangeHandler<StateId>>(
        [](auto&){ std::cout << "进入WORKING\n"; }, [](auto&){ std::cout << "退出WORKING\n"; }
    );
    auto err_h = std::make_unique<StateChangeHandler<StateId>>(
        [](auto&){ std::cout << "进入ERROR\n"; }, [](auto&){ std::cout << "退出ERROR\n"; }
    );

    std::vector<std::unique_ptr<State<StateId>>> states;
    states.push_back(std::make_unique<State<StateId>>(StateId::IDLE, std::move(idle_h)));
    states.push_back(std::make_unique<State<StateId>>(StateId::WORKING, std::move(work_h)));
    states.push_back(std::make_unique<State<StateId>>(StateId::ERROR, std::move(err_h)));

    auto fsm_h = std::make_unique<StateChangeHandler<StateId>>(
        [](auto&){ std::cout << "级联 FSM 启动\n"; }, [](auto&){ std::cout << "级联 FSM 停止\n"; }
    );
    FSM<StateId> fsm(StateId::MACHINE_ROOT, std::move(fsm_h), std::move(states));

    auto sub = fsm.subscribe([](const StateId& f, const StateId& t){
        std::cout << "状态转换: " << static_cast<int>(f) << " -> " << static_cast<int>(t) << "\n";
    });

    AutoRequirement auto1, auto2;

    // ROOT->IDLE
    fsm.AppendTransition(StateId::MACHINE_ROOT, StateId::IDLE, nullptr, nullptr, false);
    // IDLE -(auto1)-> WORKING
    fsm.AppendTransition(StateId::IDLE, StateId::WORKING, &auto1,
        [](auto,auto){ std::cout << "执行: IDLE->WORKING\n"; }, false);
    // WORKING -(auto2)-> ERROR
    fsm.AppendTransition(StateId::WORKING, StateId::ERROR, &auto2,
        [](auto,auto){ std::cout << "执行: WORKING->ERROR\n"; }, false);

    std::cout << "\n--- 启动（应自动连跳到 ERROR） ---\n";
    fsm.Start();

    std::cout << "\n--- 停止 ---\n";
    fsm.Stop();
}


int main(int, char**){
    std::cout << "开始状态机测试\n";
    std::cout << "================\n";
    
    test_basic_linear_fsm();
    test_hierarchical_fsm();
    test_self_transition_and_history();
     test_cross_branch_and_history_reentry();
    test_runtime_append_and_lca_error();
    test_cascade_auto_transitions();

    //test_error_conditions();
    
    std::cout << "\n状态机测试完成\n";

}
