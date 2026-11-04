"""Static checks that ilvo/data fits ilvo/docker-compose.yml.

    python3 -m unittest discover -s tests/docker
"""
import json
import re
import unittest
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[2]
ILVO = ROOT / "ilvo"


class ComposeConfigTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compose = yaml.safe_load((ILVO / "docker-compose.yml").read_text())
        cls.config = json.loads((ILVO / "data/config.json").read_text())
        cls.init = json.loads((ILVO / "data/redis.init.json").read_text())
        cls.addons = cls.init["system"]["ilvoAddons"]

    def test_redis_host_is_a_compose_service(self):
        self.assertIn(self.config["protocols"]["redis"]["ip"], self.compose["services"])

    def test_addon_redis_host_is_a_compose_service(self):
        for addon in self.addons:
            for env in addon["DockerConfig"].get("Env", []):
                if env.startswith("REDIS_HOST="):
                    with self.subTest(addon=addon["Name"]):
                        self.assertIn(env.split("=", 1)[1], self.compose["services"])

    def test_project_name_is_fixed(self):
        # Without it the project is named after the folder, which differs for the download
        self.assertIn("name", self.compose)

    def test_addons_join_the_compose_network(self):
        # Compose names its networks <project>_<network>
        networks = {f"{self.compose['name']}_{name}" for name in self.compose["networks"]}
        for addon in self.addons:
            with self.subTest(addon=addon["Name"]):
                self.assertIn(addon["DockerConfig"]["HostConfig"]["NetworkMode"], networks)

    def test_artof_service_mounts_the_data_folder(self):
        artof = self.compose["services"]["artof"]
        env = dict(e.split("=", 1) for e in artof["environment"])
        self.assertIn(f"./data:{env['ILVO_PATH']}", artof["volumes"])
        self.assertIn("/var/run/docker.sock:/var/run/docker.sock", artof["volumes"])

    def test_addon_binds_are_below_ilvo_path(self):
        # The system manager maps bind sources below ILVO_PATH to the host folder
        # mounted there (./data); a source elsewhere would be a host-specific path
        env = dict(e.split("=", 1) for e in self.compose["services"]["artof"]["environment"])
        ilvo_path = env["ILVO_PATH"]
        for addon in self.addons:
            for bind in addon["DockerConfig"]["HostConfig"].get("Binds", []):
                source = bind.split(":")[0]
                with self.subTest(addon=addon["Name"], bind=bind):
                    self.assertTrue(source == ilvo_path or source.startswith(ilvo_path + "/"))
                    relative = source[len(ilvo_path):].lstrip("/")
                    if relative and relative != "node-red":  # node-red/ is created at run time
                        self.assertTrue((ILVO / "data" / relative).exists())

    def test_redis_image_matches_the_package_pin(self):
        postinst = (ROOT / "artof-core/DEBIAN/postinst").read_text()
        pin = re.search(r"^REDIS_IMAGE=(\S+)", postinst, re.M).group(1)
        self.assertEqual(self.compose["services"]["redis"]["image"], pin)

    def test_published_ports_do_not_clash(self):
        ports = [p.split(":")[0] for s in self.compose["services"].values() for p in s.get("ports", [])]
        for addon in self.addons:
            for bindings in addon["DockerConfig"]["HostConfig"].get("PortBindings", {}).values():
                ports.extend(b["HostPort"] for b in bindings)
        self.assertEqual(len(ports), len(set(ports)), ports)

    def test_published_ports_are_exposed(self):
        for addon in self.addons:
            config = addon["DockerConfig"]
            for port in config["HostConfig"].get("PortBindings", {}):
                with self.subTest(addon=addon["Name"], port=port):
                    self.assertIn(port, config.get("ExposedPorts", {}))


if __name__ == "__main__":
    unittest.main()
