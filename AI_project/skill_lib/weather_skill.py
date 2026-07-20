import json

#================1. Define 2 skills(function info, pass to LLM)=================#
tools = [
    {
        "type": "function",
        "function": {
            "name": "math_calc",
            "description": "science calculator, support +-*/. called when user ask for calc, alge...",
            "parameters": {
                "type": "object",
                "properties": {
                    "expr": {
                        "type": "string",
                        "description": "math expression, e.g. 123+45*2, 10**3"
                    }
                },
                "required": ["expr"],
                "additionalProperties": False
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "get_weather",
            "description": "inquiry city real-time weather, using when user ask for temperature, rain, wind, advice for wear, support domestic cities only",
            "parameters": {
                "type": "object",
                "properties": {
                    "city": {"type": "string", "description": "Chinese city name, e.g. 北京、上海"},
                    "need_tip": {"type": "boolean", "description": "wether return wear advice, default: true", "default": True}
                },
                "required": ["city"],
                "additionalProperties": False
            }
        }
    }
]


#========================2. Skill underlying execution function(real logic)==========================#
def math_calc(expr: str):
    """math calc Skill realize"""
    try:
        # simple safe calc, use ast.literal_eval exchange eval in development env
        res = eval(expr)
        return {"code": 0, "result": res, "expr": expr}
    except Exception as e:
        return {"code": -1, "msg": f"calc failed: {str(e)}"}

def get_weather(city: str, need_tip: bool = True):
    """stimu api of weather inquiry Skill"""
    mock_db = {
        "beijing": {"temp": 29, "weather": "sunny", "wind": "south 3 level"},
        "shanghai": {"temp": 32, "weather": "cloudy", "wind": "east 2 level"},
        "guangzhou": {"temp": 35, "weather": "thumder rain", "wind": "south-east 4 level"}
    }
    data = mock_db.get(city)
    if not data:
        return {"code": -1, "msg": f"no {city} weather data"}
    # insert query city into dictionary
    data["city"] = city
    if need_tip:
        if data["temp"] >= 33:
            data["tip"] = "high temperature, wear shorts, bring sunscreen umbralla"
        elif data["temp"] <= 20:
            data["tip"] = "lower temperature, thin coat"
        else:
            data["tip"] = "comfortable temperature, normal wear"
    return {"code": 0, "data": data}

# 工具分发器: 根据LLM返回的name自动执行对应的skill
def run_skill(tool_call):
    func_name = tool_call["name"]
    args = json.loads(tool_call["arguments"])
    if func_name == "math_calc":
        return math_calc(** args)
    elif func_name == "get_weather":
        return get_weather(** args)
    else:
        return {"code": -99, "msg": f"{func_name} not exist"}


#============================3. simulate LLM interact procedure===========================#
def llm_simulate(user_query):
    """simulate LLM to decide weather call Skill, in real env alternate with http to ask LLM api"""
    print(f"user ask: {user_query}")
    # simple rule to similate LLM tool choices, real env auto output tool_call by LLM
    if any(k in user_query for k in ["cal", "equal", "multiply", "plus", "devide", "square"]):
        expr = user_query.replace("calc", "").replace("equal to", "").strip()
        tool_call = {
            "name": "math_calc",
            "arguments": json.dumps({"expr": expr})
        }
        return {"need_call_tool": True, "tool_call": tool_call}
    elif any(city in user_query for city in ["beijing", "shanghai", "guangzhou"]):
        for city in ["beijing", "shanghai", "guangzhou"]:
            if city in user_query:
                tool_call = {
                    "name": "get_weather",
                    "arguments": json.dumps({"city": city})
                }
                return {"need_call_tool": True, "tool_call": tool_call}
    else:
        return {"need_call_tool": False, "reply": f"I answer directly: {user_query}"}

#============================4. 完整主流程调度=============================
def agent_run(user_input):
    # 1, LLM check weather need to call Skill
    llm_resp = llm_simulate(user_input)
    if not llm_resp["need_call_tool"]:
        return llm_resp["reply"]

    # 2. execute corresponding Skill
    tool_result = run_skill(llm_resp["tool_call"])
    print(f"\n[Skill result] {json.dumps(tool_result, ensure_ascii=False, indent=2)}")

    # 3. return tool results to LLM, tide up NL answer(simplify concatenation)
    if tool_result["code"] != 0:
        final_ans = f"execute failed: {tool_result['msg']}"
    elif llm_resp["tool_call"]["name"] == "math_calc":
        final_ans = f"calc res: {tool_result['expr']} = {tool_result['result']}"
    elif llm_resp["tool_call"]["name"] == "get_weather":
        d = tool_result["data"]
        final_ans = f"{d['city']} today {d['weather']}, temperature {d['temp']} C, wind power {d['wind']}. Tips: {d['tip']}"
    return f"\n[AI answer]{final_ans}"


#============================5. test running==========================
if __name__ == "__main__":
    test_list = [
        "calc 128 * 5 + 30",
        "how is beijing weather today",
        "guangzhou wear tips",
        "what to eat today"
    ]
    for q in test_list:
        print("-" * 50)
        print(agent_run(q))
