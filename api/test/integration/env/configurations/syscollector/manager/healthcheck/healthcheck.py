import os
import socket
import sys

sys.path.append('/tools')

from healthcheck_utils import get_manager_health_base, check

if __name__ == "__main__":
    # Workers are not needed in this test, so the exit code is set to 0 (healthy).
    exit(check(os.system(
        "grep -q 'blackwell-modulesd:syscollector: INFO: Evaluation finished.' /var/ossec/logs/ossec.log"))
         or get_manager_health_base(
        env_mode=sys.argv[1] if len(sys.argv) > 1 else None)) if socket.gethostname() == 'blackwell-master' else exit(0)
