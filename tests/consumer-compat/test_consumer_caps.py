import importlib.util, json, pathlib, unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("capture",ROOT/"scripts/capture-consumer-capabilities.py"); capture=importlib.util.module_from_spec(spec);spec.loader.exec_module(capture)

class CaptureTests(unittest.TestCase):
 def test_fixture_schema(self):
  data=json.loads((ROOT/"validation/g615-v11-csf/consumer-capabilities.json").read_text()); self.assertIs(capture.validate(data),data)
 def test_rejects_wrong_device(self):
  data=json.loads((ROOT/"validation/g615-v11-csf/consumer-capabilities.json").read_text());data["device"]["deviceID"]=1
  with self.assertRaises(ValueError):capture.validate(data)
 def test_probe_covers_required_calls(self):
  source=(ROOT/"tests/consumer-compat/g615_consumer_caps.c").read_text()
  for name in ("vkGetPhysicalDeviceFeatures2","vkGetPhysicalDeviceProperties2","vkGetPhysicalDeviceQueueFamilyProperties","vkGetPhysicalDeviceFormatProperties","vkGetPhysicalDeviceFormatProperties2","vkGetPhysicalDeviceImageFormatProperties2"):self.assertIn(name,source)
 def test_rejects_incomplete_structure(self):
  data=json.loads((ROOT/"validation/g615-v11-csf/consumer-capabilities.json").read_text()); data["featureStructures"].pop(next(iter(data["featureStructures"])))
  with self.assertRaisesRegex(ValueError,"structure coverage"):capture.validate(data)
 def test_rejects_incomplete_core_properties(self):
  data=json.loads((ROOT/"validation/g615-v11-csf/consumer-capabilities.json").read_text()); data["coreProperties"]["limits"].pop(next(iter(data["coreProperties"]["limits"])))
  with self.assertRaisesRegex(ValueError,"field coverage"):capture.validate(data)
 def test_generated_probe_compiles_complete_inventory(self):
  data=json.loads((ROOT/"validation/g615-v11-csf/consumer-capabilities.json").read_text())
  source=capture.generated_probe_source((ROOT/"tests/consumer-compat/g615_consumer_caps.c").read_text(),{x["name"] for x in data["extensions"]})
  expected=capture.registry_inventory({x["name"] for x in data["extensions"]})
  for section in expected:
   for structure,meta in expected[section].items():
    self.assertIn(structure,source)
    for field in meta["fields"]:self.assertIn(f'json_string("{field}")',source)

if __name__=="__main__":unittest.main()
