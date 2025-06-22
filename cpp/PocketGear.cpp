/**
 * Love From YuWu 2025.06.22
 * ✨ YuWuの背包魔法 ✨
 */

#include "efmod-api/efmod_core.hpp"
#include "tefmod-api/TEFMod.hpp"
#include "tefmod-api/BaseType.hpp"

/*
 * 玩家背包相关指针
 */
TEFMod::Field<void*>* g_pInventory;      // 主背包物品列表
TEFMod::Field<void*>* g_pBankItemBase;   // 银行物品基础指针

/**
 * 各类银行容器
 */
TEFMod::Field<void*>* g_pBank;          // 普通银行
TEFMod::Field<void*>* g_pBank2;         // 保险箱
TEFMod::Field<void*>* g_pBank3;         // 护卫保险箱
TEFMod::Field<void*>* g_pBank4;         // 虚空保险库

/*
 * 装备效果应用方法
 */
TEFMod::Method<void>* g_pApplyEquipFunc;    // 应用装备功能效果
TEFMod::Method<void>* g_pGrantPrefixBonus;  // 应用前缀加成
TEFMod::Method<void>* g_pGrantArmorBonus;   // 应用护甲加成

/*
 * 类型解析器
 */
static TEFMod::Field<int>* (*ParseIntField)(void*);       // 整型字段解析器
static TEFMod::Field<bool>* (*ParseBoolField)(void*);     // 布尔字段解析器
static TEFMod::Field<void*>* (*ParseObjField)(void*);     // 对象字段解析器
static TEFMod::Field<float>* (*ParseFloatField)(void*);   // 浮点字段解析器

static TEFMod::Method<int>* (*ParseIntMethod)(void*);     // 整型方法解析器
static TEFMod::Method<void>* (*ParseVoidMethod)(void*);   // 无返回值方法解析器

static TEFMod::Array<int>* (*ParseIntArray)(void*);       // 整型数组解析器
static TEFMod::Array<bool>* (*ParseBoolArray)(void*);     // 布尔数组解析器
static TEFMod::Array<void*>* (*ParseObjArray)(void*);     // 对象数组解析器

/*
 * 原始函数指针
 */
void (*g_pOriginalResetEffects)(TEFMod::TerrariaInstance);  // 原版ResetEffects

/**
 * ResetEffects钩子模板
 * @param player 玩家实例
 */
void HookTemplate_ResetEffects(TEFMod::TerrariaInstance player);

// 钩子模板配置
static TEFMod::HookTemplate g_ResetEffectsHook = {
    reinterpret_cast<void*>(HookTemplate_ResetEffects),
    {}
};

// 前置声明
void ApplyPocketEffects(TEFMod::TerrariaInstance player);
void ProcessInventory(TEFMod::TerrariaInstance player, TEFMod::Array<void*>* pItems);

/**
 * @class PocketGear
 * @brief 口袋装备核心实现
 */
class PocketGear final : public EFMod {
public:
    // 加载Mod(暂不实现)
    int Load(const std::string& path, MultiChannel* pChannel) override;

    // 卸载Mod(暂不实现)
    int UnLoad(const std::string& path, MultiChannel* pChannel) override;

    // 注册API和钩子
    void Send(const std::string& path, MultiChannel* pChannel) override;

    // 获取API和初始化
    void Receive(const std::string& path, MultiChannel* pChannel) override;

    // 获取Mod信息
    Metadata GetMetadata() override;

    // 初始化Mod(暂不实现)
    int Initialize(const std::string& path, MultiChannel* pChannel) override;
};

/*
 * 空实现函数
 */
int PocketGear::Load(const std::string&, MultiChannel*) { return 0; }
int PocketGear::UnLoad(const std::string&, MultiChannel*) { return 0; }
int PocketGear::Initialize(const std::string&, MultiChannel*) { return 0; }

/**
 * 获取Mod元数据
 */
Metadata PocketGear::GetMetadata() {
    return {
        "口袋装备",         // Mod名称
        "雨鹜",            // 作者
        "1.0.0",          // 版本
        20250622,         // 构建日期
        ModuleType::Game, // 类型
        { false }         // 额外标记
    };
}

/**
 * ResetEffects钩子模板
 * 先调用原版函数，再执行所有注册的钩子
 */
void HookTemplate_ResetEffects(TEFMod::TerrariaInstance player) {
    g_pOriginalResetEffects(player);
    for (const auto pFunc : g_ResetEffectsHook.FunctionArray) {
        if (pFunc) {
            reinterpret_cast<void(*)(TEFMod::TerrariaInstance)>(pFunc)(player);
        }
    }
}

/**
 * 应用口袋装备效果
 * 处理玩家所有背包中的装备
 */
void ApplyPocketEffects(TEFMod::TerrariaInstance player) {
    // 处理主背包
    ProcessInventory(player, ParseObjArray(g_pInventory->Get(player)));

    // 处理各类银行
    ProcessInventory(player, ParseObjArray(g_pBankItemBase->Get(g_pBank->Get(player))));
    ProcessInventory(player, ParseObjArray(g_pBankItemBase->Get(g_pBank2->Get(player))));
    ProcessInventory(player, ParseObjArray(g_pBankItemBase->Get(g_pBank3->Get(player))));
    ProcessInventory(player, ParseObjArray(g_pBankItemBase->Get(g_pBank4->Get(player))));
}

/**
 * 处理单个物品栏
 * 为每个物品应用装备效果
 */
void ProcessInventory(TEFMod::TerrariaInstance player, TEFMod::Array<void*>* pItems) {
    for (int i = 0; i < pItems->Size() - 1; ++i) {
        const auto pItem = pItems->at(i);
        g_pApplyEquipFunc->Call(player, 2, i, pItem);      // 应用装备功能
        g_pGrantPrefixBonus->Call(player, 1, pItem);       // 应用前缀加成
        g_pGrantArmorBonus->Call(player, 1, pItem);        // 应用护甲加成
    }
}

/**
 * 注册API和钩子
 */
void PocketGear::Send(const std::string& path, MultiChannel* pChannel) {
    auto pAPI = pChannel->receive<TEFMod::TEFModAPI*>("TEFMod::TEFModAPI");

    // 注册所有背包字段
    const char* szFields[] = { "inventory", "bank", "bank2", "bank3", "bank4" };
    for (const auto szField : szFields) {
        pAPI->registerApiDescriptor({
            "Terraria",  // 命名空间
            "Player",    // 类名
            szField,     // 字段名
            "Field"      // 类型
        });
    }

    // 注册银行物品字段
    pAPI->registerApiDescriptor({
       "Terraria",
       "Chest",
       "item",
       "Field"
    });

    // 注册装备效果方法
    pAPI->registerApiDescriptor({
        "Terraria",
        "Player",
        "ApplyEquipFunctional",
        "Method",
        2
    });

    pAPI->registerApiDescriptor({
        "Terraria",
        "Player",
        "GrantPrefixBenefits",
        "Method",
        1
    });

    pAPI->registerApiDescriptor({
        "Terraria",
        "Player",
        "GrantArmorBenefits",
        "Method",
        1
    });

    // 注册ResetEffects钩子
    pAPI->registerFunctionDescriptor({
        "Terraria",
        "Player",
        "ResetEffects",
        "hook>>void",
        0,
        &g_ResetEffectsHook,
        { reinterpret_cast<void*>(ApplyPocketEffects) }
    });
}

/**
 * 初始化API和指针
 */
void PocketGear::Receive(const std::string& path, MultiChannel* pChannel) {
    // 获取解析器
    ParseIntField = pChannel->receive<decltype(ParseIntField)>("TEFMod::Field<Int>::ParseFromPointer");
    ParseBoolField = pChannel->receive<decltype(ParseBoolField)>("TEFMod::Field<Bool>::ParseFromPointer");
    ParseObjField = pChannel->receive<decltype(ParseObjField)>("TEFMod::Field<Other>::ParseFromPointer");
    ParseFloatField = pChannel->receive<decltype(ParseFloatField)>("TEFMod::Field<Float>::ParseFromPointer");

    ParseIntMethod = pChannel->receive<decltype(ParseIntMethod)>("TEFMod::Method<Int>::ParseFromPointer");
    ParseVoidMethod = pChannel->receive<decltype(ParseVoidMethod)>("TEFMod::Method<Void>::ParseFromPointer");

    ParseIntArray = pChannel->receive<decltype(ParseIntArray)>("TEFMod::Array<Int>::ParseFromPointer");
    ParseBoolArray = pChannel->receive<decltype(ParseBoolArray)>("TEFMod::Array<Bool>::ParseFromPointer");
    ParseObjArray = pChannel->receive<decltype(ParseObjArray)>("TEFMod::Array<Other>::ParseFromPointer");

    // 获取API实例
    auto pAPI = pChannel->receive<TEFMod::TEFModAPI*>("TEFMod::TEFModAPI");

    // 获取原版ResetEffects
    g_pOriginalResetEffects = pAPI->GetAPI<decltype(g_pOriginalResetEffects)>({
        "Terraria",
        "Player",
        "ResetEffects",
        "old_fun",
        0
    });

    // 初始化背包相关指针
    g_pBankItemBase = ParseObjField(pAPI->GetAPI<void*>({
        "Terraria", "Chest", "item", "Field"
    }));

    g_pInventory = ParseObjField(pAPI->GetAPI<void*>({
        "Terraria", "Player", "inventory", "Field"
    }));

    g_pBank = ParseObjField(pAPI->GetAPI<void*>({
        "Terraria", "Player", "bank", "Field"
    }));

    g_pBank2 = ParseObjField(pAPI->GetAPI<void*>({
        "Terraria", "Player", "bank2", "Field"
    }));

    g_pBank3 = ParseObjField(pAPI->GetAPI<void*>({
        "Terraria", "Player", "bank3", "Field"
    }));

    g_pBank4 = ParseObjField(pAPI->GetAPI<void*>({
        "Terraria", "Player", "bank4", "Field"
    }));

    // 初始化方法指针
    g_pApplyEquipFunc = ParseVoidMethod(pAPI->GetAPI<void*>({
        "Terraria", "Player", "ApplyEquipFunctional", "Method", 2
    }));

    g_pGrantPrefixBonus = ParseVoidMethod(pAPI->GetAPI<void*>({
        "Terraria", "Player", "GrantPrefixBenefits", "Method", 1
    }));

    g_pGrantArmorBonus = ParseVoidMethod(pAPI->GetAPI<void*>({
        "Terraria", "Player", "GrantArmorBenefits", "Method", 1
    }));
}

/**
 * 获取Mod实例
 */
EFMod* CreateMod() {
    static PocketGear instance;
    return &instance;
}