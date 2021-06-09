#include "../syncthingtestinstance.h"

#include <c++utilities/tests/testutils.h>

#include <iostream>

using namespace std;
using namespace CppUtilities;

/*!
 * \brief Launches a Syncthing test instance for manual testing.
 */
int main(int argc, char **argv)
{
    TestApplication testApp(argc, argv);
    if (!testApp) {
        return -1;
    }

    SyncthingTestInstance testInstance;
    auto &syncthingProcess(testInstance.syncthingProcess());
    //syncthingProcess.setProcessChannelMode(QProcess::ForwardedChannels);
    QObject::connect(&syncthingProcess, static_cast<void (Data::SyncthingProcess::*)(int, QProcess::ExitStatus)>(&Data::SyncthingProcess::finished),
        &QCoreApplication::exit);
    testInstance.start();

    const int res = testInstance.application().exec();
    testInstance.stop();
    return res;
}
