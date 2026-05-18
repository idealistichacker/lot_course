from rag import add_knowledge


greenhouse_knowledge = [
    {
        "source": "initial_seed",
        "crop": "番茄",
        "type": "greenhouse",
        "text": "番茄大棚适宜温度一般白天约20-30℃，温度偏高时应优先考虑通风、遮阴或喷雾降温。若温度超过35℃，容易影响授粉和生长。",
    },
    {
        "source": "initial_seed",
        "crop": "番茄",
        "type": "greenhouse",
        "text": "番茄大棚土壤湿度偏低时可以补水，但应避免一次性过量浇水。补水后应观察5分钟到30分钟内土壤湿度变化。",
    },
    {
        "source": "initial_seed",
        "crop": "番茄",
        "type": "greenhouse",
        "text": "大棚二氧化碳浓度过高或空气湿度过高时，应通过通风降低闷棚风险。通风时间可从5到20分钟开始，根据温湿度变化调整。",
    },
]

indoor_pot_knowledge = [
    {
        "source": "initial_seed",
        "crop": "绿萝",
        "type": "indoor_pot",
        "text": "室内盆栽浇水应根据盆土体积和土壤湿度控制。小盆不宜一次浇水过多，可以先少量补水，观察土壤湿度变化。",
    },
    {
        "source": "initial_seed",
        "crop": "绿萝",
        "type": "indoor_pot",
        "text": "绿萝适合温暖湿润环境，但盆土长期积水容易烂根。室内盆栽更应关注土壤湿度，而不是单纯根据空气湿度浇水。",
    },
]

general_knowledge = [
    {
        "source": "initial_seed",
        "type": "general",
        "text": "农业控制建议应避免连续频繁操作。执行浇水、通风或降温后，应等待约5分钟观察环境变化，再决定是否继续操作。",
    },
    {
        "source": "initial_seed",
        "type": "general",
        "text": "当传感器数据缺失或异常时，应优先选择无操作，并提示人工检查传感器。自动控制不应在数据不完整时进行激进操作。",
    },
]

add_knowledge(greenhouse_knowledge, category="greenhouse")
add_knowledge(indoor_pot_knowledge, category="indoor_pot")
add_knowledge(general_knowledge, category="general")

print("✅ 初始知识库已构建完成")