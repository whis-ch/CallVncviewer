using VncMiddleware;

namespace RemoteView
{
    public class RemoteView
    {
        private IRemoteViewer remoteViewer { get; set; }

        public RemoteView() => this.remoteViewer = new RemoteViewer();

    }
}
