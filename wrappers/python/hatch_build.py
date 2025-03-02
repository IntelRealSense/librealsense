from hatchling.builders.hooks.plugin.interface import BuildHookInterface

# With infer_tag = True, Hatchling will automatically infer the tag
# based on the current environment (the platform, Python version, etc.)
class CustomBuildHook(BuildHookInterface):
    def initialize(self, version, build_data):
        build_data['infer_tag'] = True
