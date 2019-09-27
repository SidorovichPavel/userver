#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include <engine/subprocess/child_process.hpp>
#include <engine/subprocess/environment_variables.hpp>

namespace engine {

class TaskProcessor;

namespace ev {
class ThreadControl;
}  // namespace ev

namespace subprocess {

class ProcessStarter {
 public:
  explicit ProcessStarter(TaskProcessor& task_processor);

  /// `env` redefines all environment variables.
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      const EnvironmentVariables& env,
      // TODO: use something like pipes instead of path to files
      const boost::optional<std::string>& stdout_file = boost::none,
      const boost::optional<std::string>& stderr_file = boost::none);

  /// Variables from `env_update` will be added to current environment.
  /// Existing values will be replaced.
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      EnvironmentVariablesUpdate env_update,
      // TODO: use something like pipes instead of path to files
      const boost::optional<std::string>& stdout_file = boost::none,
      const boost::optional<std::string>& stderr_file = boost::none);

  /// Exec subprocess using current environment.
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      // TODO: use something like pipes instead of path to files
      const boost::optional<std::string>& stdout_file = boost::none,
      const boost::optional<std::string>& stderr_file = boost::none);

 private:
  ev::ThreadControl& thread_control_;
};

}  // namespace subprocess
}  // namespace engine
