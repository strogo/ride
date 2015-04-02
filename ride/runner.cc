#include "ride/runner.h"
#include <wx/utils.h>
#include <wx/process.h>
#include <wx/txtstrm.h>

#include "ride/mainwindow.h"

class PipedProcess;
class Process;

struct Runner::Pimpl {
  Pimpl() : processes_(0) {
  }

  void Append(const wxString&) {}
  void AddProcess(Process *process);
  void ProcessTerminated(Process *process);

  int RunCmd(const wxString& root, const wxString& cmd);

  Process* processes_;
};

class Process : public wxProcess
{
public:
  Process(Runner::Pimpl* project, const wxString& cmd)
    : wxProcess(), cmd_(cmd)
  {
    runner_ = project;
    Redirect();
  }

  virtual void OnTerminate(int pid, int status) {
    // show the rest of the output
    while (HasInput()) {}
    runner_->ProcessTerminated(this);

    runner_->Append(wxString::Format(wxT("Process %u ('%s') terminated with exit code %d."),
      pid, cmd_.c_str(), status));
    runner_->Append("");
    runner_->ProcessTerminated(this);
  }

  virtual bool HasInput() {
    bool hasInput = false;

    if (IsInputAvailable())
    {
      wxTextInputStream tis(*GetInputStream());
      const wxString msg = tis.ReadLine(); // this assumes that the output is always line buffered
      runner_->Append(msg);
      hasInput = true;
    }

    if (IsErrorAvailable())
    {
      wxTextInputStream tis(*GetErrorStream());
      const wxString msg = tis.ReadLine(); // this assumes that the output is always line buffered
      runner_->Append(msg);
      hasInput = true;
    }

    return hasInput;
  }

protected:
  Runner::Pimpl *runner_;
  wxString cmd_;
};

int Runner::Pimpl::RunCmd(const wxString& root, const wxString& cmd) {
  bool async = true;

  Process* process = new Process(this, cmd);
  Append("> " + cmd);

  wxExecuteEnv env;
  env.cwd = root;

  const int flags = async
    ? wxEXEC_ASYNC | wxEXEC_SHOW_CONSOLE
    : wxEXEC_SYNC | wxEXEC_SHOW_CONSOLE;

  const int execute_result = wxExecute(cmd, flags, process, &env);
  
  if (async) {
    // async call
    if (!execute_result) {
      Append(wxString::Format(wxT("Execution of '%s' failed."), cmd.c_str()));
      delete process;
    }
    else {
      AddProcess(process);
    }
  }
  else {
    // sync call, remove process
    delete process;
  }

  return execute_result;
}

void Runner::Pimpl::AddProcess(Process *process)
{
  assert(processes_ == NULL);
  processes_ = process;
}

void Runner::Pimpl::ProcessTerminated(Process *process)
{
  assert(processes_ == process);
  processes_ = process;
  delete process;
}

//////////////////////////////////////////////////////////////////////////

Runner::Runner() : pimpl(new Pimpl()) {
}
Runner::~Runner() {
}

bool Runner::RunCmd(const wxString& root, const wxString& cmd) {
  return false;
}
bool Runner::IsRunning() const {
  return true;
}
int Runner::GetExitCode() {
  return -1;
}

