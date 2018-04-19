#include "SubmarineFree.hpp"

struct AG_106 : Module {
	static const int deviceCount = 6;
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_A_1,
		INPUT_A_2,
		INPUT_A_3,
		INPUT_A_4,
		INPUT_A_5,
		INPUT_A_6,
		INPUT_B_1,
		INPUT_B_2,
		INPUT_B_3,
		INPUT_B_4,
		INPUT_B_5,
		INPUT_B_6,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTPUT_1,
		OUTPUT_2,
		OUTPUT_3,
		OUTPUT_4,
		OUTPUT_5,
		OUTPUT_6,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	AG_106() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
};

void AG_106::step() {
	for (int i = 0; i < deviceCount; i++) {
		outputs[OUTPUT_1 + i].value = ((inputs[INPUT_A_1 + i].value > 2.5f) && (inputs[INPUT_B_1 + i].value > 2.5f)) * 5.0f;
	}
}

struct AG106 : ModuleWidget {
	AG106(AG_106 *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/AG-106.svg")));

		for (int i = 0; i < AG_106::deviceCount; i++) {
			int offset = 58 * i;
			addInput(Port::create<sub_port_blue>(Vec(3.5,19 + offset), Port::INPUT, module, AG_106::INPUT_A_1 + i));
			addInput(Port::create<sub_port_blue>(Vec(3.5,46 + offset), Port::INPUT, module, AG_106::INPUT_B_1 + i));

			addOutput(Port::create<sub_port_blue>(Vec(61.5,32.5 + offset), Port::OUTPUT, module, AG_106::OUTPUT_1 + i));
		}
	}
};

Model *modelAG106 = Model::create<AG_106, AG106>("SubmarineFree", "AG-106", "AG-106 And Gates", LOGIC_TAG, MULTIPLE_TAG);