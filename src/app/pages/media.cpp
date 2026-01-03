#include <fileref.h>
#include <math.h>
#include <tag.h>
#include <tpropertymap.h>
#include <BluezQt/PendingCall>
#include <QDirIterator>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMediaPlaylist>

#include "app/window.hpp"
#include "app/pages/media.hpp"

MediaPage::MediaPage(Arbiter &arbiter, QWidget *parent)
    : QTabWidget(parent)
    , Page(arbiter, "Media", "play_circle_outline", true, this)
{
    
}

void MediaPage::init()
{
    this->addTab(new BluetoothPlayerTab(this->arbiter, this), "Bluetooth");
    this->addTab(new LocalPlayerTab(this->arbiter, this), "Local");

    QIcon icon;
    const QString p = ":/icons/play_circle_outline.svg"; // put your coloured SVG here
    icon.addFile(p, QSize(), QIcon::Normal, QIcon::Off);
    icon.addFile(p, QSize(), QIcon::Normal, QIcon::On);
    icon.addFile(p, QSize(), QIcon::Active, QIcon::Off);
    icon.addFile(p, QSize(), QIcon::Active, QIcon::On);
    this->button()->setIcon(icon);
}

BluetoothPlayerTab::BluetoothPlayerTab(Arbiter &arbiter, QWidget *parent)
    : QWidget(parent)
    , arbiter(arbiter)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(this->track_widget());
    layout->addWidget(this->controls_widget());
}

QWidget *BluetoothPlayerTab::track_widget()
{
    BluezQt::MediaPlayerPtr media_player = this->arbiter.system().bluetooth.get_media_player().second;
    AAHandler *aa_handler = this->arbiter.android_auto().handler;

    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);

    QLabel *artist_hdr = new QLabel("Artist", widget);
    QLabel *artist = new QLabel((media_player != nullptr) ? media_player->track().artist() : QString(), widget);
    artist->setIndent(16);
    layout->addWidget(artist_hdr);
    layout->addWidget(artist);

    QLabel *album_hdr = new QLabel("Album", widget);
    QLabel *album = new QLabel((media_player != nullptr) ? media_player->track().album() : QString(), widget);
    album->setIndent(16);
    layout->addWidget(album_hdr);
    layout->addWidget(album);

    QLabel *title_hdr = new QLabel("Title", widget);
    QLabel *title = new QLabel((media_player != nullptr) ? media_player->track().title() : QString(), widget);
    title->setIndent(16);
    layout->addWidget(title_hdr);
    layout->addWidget(title);

    QLabel *albumArt = new QLabel(widget);
    layout->addWidget(albumArt);

    connect(&this->arbiter.system().bluetooth, &Bluetooth::media_player_track_changed, [this, artist, album, title](BluezQt::MediaPlayerTrack track){
        artist->setText(track.artist());
        album->setText(track.album());
        title->setText(track.title());
        const QString a = track.artist();
        const QString t = track.title();
        const QString np = (!a.isEmpty() && !t.isEmpty()) ? (a + " — " + t)
                          : (!t.isEmpty() ? t : QString());
        this->arbiter.set_now_playing("BT", np, !np.isEmpty());

    });
    connect(aa_handler, &AAHandler::aa_media_metadata_update, [artist, album, title, albumArt](const aasdk::proto::messages::MediaInfoChannelMetadataData& metadata){
        title->setText(QString::fromStdString(metadata.track_name()));
        if(metadata.has_artist_name()) artist->setText(QString::fromStdString(metadata.artist_name()));
        if(metadata.has_album_name()) album->setText(QString::fromStdString(metadata.album_name()));
        if(metadata.has_album_art()){
            QImage art;
            art.loadFromData(QByteArray::fromStdString(metadata.album_art()));
            albumArt->setPixmap(QPixmap::fromImage(art));
        }
    
    });


    return widget;
}

QWidget *BluetoothPlayerTab::controls_widget()
{
    BluezQt::MediaPlayerPtr media_player = this->arbiter.system().bluetooth.get_media_player().second;

    QWidget *widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);

    QPushButton *previous_button = new QPushButton(widget);
    previous_button->setFlat(true);
    this->arbiter.forge().iconize("skip_previous", previous_button, 56);
    connect(previous_button, &QPushButton::clicked, [this]{
        BluezQt::MediaPlayerPtr media_player = this->arbiter.system().bluetooth.get_media_player().second;
        if (media_player != nullptr)
            media_player->previous()->waitForFinished();
    });
    layout->addWidget(previous_button);

    QPushButton *play_button = new QPushButton(widget);
    play_button->setFlat(true);
    play_button->setCheckable(true);
    bool status = (media_player != nullptr) ? media_player->status() == BluezQt::MediaPlayer::Status::Playing : false;
    play_button->setChecked(status);
    this->arbiter.forge().iconize("play", "pause", play_button, 56);
    connect(play_button, &QPushButton::clicked, [this, play_button](bool checked = false){
        play_button->setChecked(!checked);

        BluezQt::MediaPlayerPtr media_player = this->arbiter.system().bluetooth.get_media_player().second;
        if (media_player != nullptr) {
            if (checked)
                media_player->play()->waitForFinished();
            else
                media_player->pause()->waitForFinished();
        }
    });
    connect(&this->arbiter.system().bluetooth, &Bluetooth::media_player_status_changed, [this, play_button](BluezQt::MediaPlayer::Status status){
        play_button->setChecked(status == BluezQt::MediaPlayer::Status::Playing);
        // If BT is stopped, dim the Now Playing strip (keep last text if you prefer)
        if (status == BluezQt::MediaPlayer::Stopped) {
            this->arbiter.set_now_playing("BT", "", false);
        }
    });
    layout->addWidget(play_button);

    QPushButton *forward_button = new QPushButton(widget);
    forward_button->setFlat(true);
    this->arbiter.forge().iconize("skip_next", forward_button, 56);
    connect(forward_button, &QPushButton::clicked, [this]{
        BluezQt::MediaPlayerPtr media_player = this->arbiter.system().bluetooth.get_media_player().second;
        if (media_player != nullptr)
            media_player->next()->waitForFinished();
    });
    layout->addWidget(forward_button);

    return widget;
}

LocalPlayerTab::LocalPlayerTab(Arbiter &arbiter, QWidget *parent)
    : QWidget(parent)
    , arbiter(arbiter)
{
    this->config = Config::get_instance();

    QMediaPlaylist *playlist = new QMediaPlaylist(this);
    playlist->setPlaybackMode(QMediaPlaylist::Loop);

    this->player = new QMediaPlayer(this);
    this->player->setPlaylist(playlist);

    this->path_label = new QLabel(this->config->get_media_home(), this);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(this->path_label);
    layout->addWidget(this->playlist_widget());
    layout->addWidget(this->seek_widget());
    layout->addWidget(this->controls_widget());
}

QWidget *LocalPlayerTab::playlist_widget()
{
    auto player = this->player;
    auto config = this->config;

    QWidget *widget = new QWidget;
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    // --- NOW PLAYING (top card) ---
    QFrame *now = new QFrame;
    now->setObjectName("LocalNowPlaying");
    auto nowLayout = new QVBoxLayout(now);
    nowLayout->setContentsMargins(14, 12, 14, 12);
    nowLayout->setSpacing(4);

    QLabel *npTag = new QLabel("LOCAL");
    npTag->setObjectName("LocalNowPlayingTag");

    QLabel *npTitle = new QLabel("Nothing playing");
    npTitle->setObjectName("LocalNowPlayingTitle");
    npTitle->setWordWrap(true);

    QLabel *npArtist = new QLabel("");
    npArtist->setObjectName("LocalNowPlayingArtist");
    npArtist->setWordWrap(true);

    QLabel *npAlbum = new QLabel("");
    npAlbum->setObjectName("LocalNowPlayingAlbum");
    npAlbum->setWordWrap(true);

    nowLayout->addWidget(npTag);
    nowLayout->addWidget(npTitle);
    nowLayout->addWidget(npArtist);
    nowLayout->addWidget(npAlbum);

    layout->addWidget(now);

    // --- TRACK LIST (single folder) ---
    QListWidget *tracks = new QListWidget;
    tracks->setObjectName("LocalTrackList");
    tracks->setSelectionMode(QAbstractItemView::SingleSelection);

    layout->addWidget(tracks, 1);

    // Load tracks from ONE folder only
    const QString root_path = config->get_media_home();
    tracks->clear();
    player->playlist()->clear();
    this->populate_tracks(root_path, tracks);

    // When user taps a track, play it
    QObject::connect(tracks, &QListWidget::currentRowChanged, this,
    [this, player, tracks, npTitle, npArtist](int i) {
        if (i < 0)
            return;

        player->playlist()->setCurrentIndex(i);

        // pull metadata back out of the list item
        auto *item = tracks->item(i);
        const QString artist = item ? item->data(Qt::UserRole + 1).toString() : QString();
        const QString song   = item ? item->data(Qt::UserRole + 2).toString() : QString();
        const QString disp   = item ? item->text() : QString();

        // update the on-page “Now Playing” card
        npTitle->setText(!song.isEmpty() ? song : disp);
        npArtist->setText(!artist.isEmpty() ? artist : QStringLiteral("Local"));

        // keep the statusbar Now Playing alive for Local too
        this->arbiter.set_now_playing(QStringLiteral("LOCAL"), disp);
    });


    // Keep Now Playing card updated as playback index changes
    auto updateNowPlayingFromIndex = [=](int idx) {
        if (idx < 0 || idx >= tracks->count()) {
            npTitle->setText("Nothing playing");
            npArtist->setText("");
            npAlbum->setText("");
            return;
        }

        QListWidgetItem *it = tracks->item(idx);

        // We’ll fill these roles in Step 2 (metadata)
        const QString title  = it->data(Qt::UserRole + 1).toString();
        const QString artist = it->data(Qt::UserRole + 2).toString();
        const QString album  = it->data(Qt::UserRole + 3).toString();

        // Fallbacks if not set yet
        npTitle->setText(title.isEmpty() ? it->text() : title);
        npArtist->setText(artist);
        npAlbum->setText(album);
    };

    QObject::connect(player->playlist(), &QMediaPlaylist::currentIndexChanged,
                     widget, [=](int idx) {
                         updateNowPlayingFromIndex(idx);
                         if (idx >= 0 && idx < tracks->count()) {
                             tracks->setCurrentRow(idx);
                         }
                     });

    // Initialise card once at boot
    updateNowPlayingFromIndex(player->playlist()->currentIndex());

    return widget;
}


QWidget *LocalPlayerTab::seek_widget()
{
    QWidget *widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);

    QSlider *slider = new QSlider(Qt::Orientation::Horizontal, widget);
    slider->setTracking(false);
    slider->setRange(0, 0);
    QLabel *value = new QLabel(LocalPlayerTab::durationFmt(slider->value()), widget);
    connect(slider, &QSlider::sliderReleased,
            [player = this->player, slider]() { player->setPosition(slider->sliderPosition()); });
    connect(slider, &QSlider::sliderMoved,
            [value](int position) { value->setText(LocalPlayerTab::durationFmt(position)); });
    connect(this->player, &QMediaPlayer::durationChanged, [slider](qint64 duration) {
        slider->setValue(0);
        slider->setRange(0, duration);
    });
    connect(this->player, &QMediaPlayer::positionChanged, [slider, value](qint64 position) {
        if (!slider->isSliderDown()) {
            slider->setValue(position);
            value->setText(LocalPlayerTab::durationFmt(position));
        }
    });

    layout->addStretch(4);
    layout->addWidget(slider, 28);
    layout->addWidget(value, 3);
    layout->addStretch(1);

    return widget;
}

QWidget *LocalPlayerTab::controls_widget()
{
    QWidget *widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);

    QPushButton *previous_button = new QPushButton(widget);
    previous_button->setFlat(true);
    this->arbiter.forge().iconize("skip_previous", previous_button, 56);
    connect(previous_button, &QPushButton::clicked, [player = this->player]() {
        if (player->playlist()->currentIndex() < 0) player->playlist()->setCurrentIndex(0);
        player->playlist()->previous();
        player->play();
    });
    layout->addWidget(previous_button);

    QPushButton *play_button = new QPushButton(widget);
    play_button->setFlat(true);
    play_button->setCheckable(true);
    play_button->setChecked(false);
    this->arbiter.forge().iconize("play", "pause", play_button, 56);
    connect(play_button, &QPushButton::clicked, [player = this->player, play_button](bool checked = false) {
        play_button->setChecked(!checked);
        if (checked)
            player->play();
        else
            player->pause();
    });
    connect(this->player, &QMediaPlayer::stateChanged,
            [play_button](QMediaPlayer::State state) { play_button->setChecked(state == QMediaPlayer::PlayingState); });
    layout->addWidget(play_button);

    QPushButton *forward_button = new QPushButton(widget);
    forward_button->setFlat(true);
    this->arbiter.forge().iconize("skip_next", forward_button, 56);
    connect(forward_button, &QPushButton::clicked, [player = this->player]() {
        player->playlist()->next();
        player->play();
    });
    layout->addWidget(forward_button);

    return widget;
}

QString LocalPlayerTab::durationFmt(int total_ms)
{
    int mins = (total_ms / (1000 * 60)) % 60;
    int secs = (total_ms / 1000) % 60;

    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void LocalPlayerTab::populate_dirs(QString path, QListWidget *dirs_widget)
{
    dirs_widget->clear();
    QDir current_dir(path);
    QFileInfoList dirs = current_dir.entryInfoList(QDir::AllDirs | QDir::Readable);
    for (QFileInfo dir : dirs) {
        if (dir.fileName() == ".") continue;

        QListWidgetItem *item = new QListWidgetItem(dir.fileName(), dirs_widget);
        if (dir.fileName() == "..") {
            item->setText("↲");

            if (current_dir.isRoot()) item->setFlags(Qt::NoItemFlags);
        }
        else {
            item->setText(dir.fileName());
        }
        item->setData(Qt::UserRole, QVariant(dir.absoluteFilePath()));
    }
}

void LocalPlayerTab::populate_tracks(QString path, QListWidget *tracks_widget)
{
    if (!tracks_widget)
        return;

    tracks_widget->clear();

    auto *pl = this->player->playlist();
    if (pl)
        pl->clear();

    // common audio extensions (add/remove as you like)
    const QStringList exts = {"mp3", "flac", "wav", "ogg", "m4a", "aac", "opus"};

    // helper to convert TagLib strings safely
    auto tl = [](const TagLib::String &s) -> QString {
        const char *c = s.toCString(true);
        return (c && *c) ? QString::fromUtf8(c) : QString();
    };

    struct TrackInfo {
        QString display;
        QString artist;
        QString title;
        QString file;
    };

    QVector<TrackInfo> tracks;

    // recursive scan
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        const QFileInfo fi(file);

        const QString ext = fi.suffix().toLower();
        if (!exts.contains(ext))
            continue;

        QString artist, title;

        TagLib::FileRef ref(file.toUtf8().constData());
        if (!ref.isNull() && ref.tag()) {
            artist = tl(ref.tag()->artist());
            title  = tl(ref.tag()->title());
        }

        QString display;
        if (!artist.isEmpty() && !title.isEmpty())
            display = artist + " — " + title;
        else if (!title.isEmpty())
            display = title;
        else
            display = fi.completeBaseName();

        tracks.push_back({display, artist, title, file});
    }

    // stable sort by display text
    std::sort(tracks.begin(), tracks.end(), [](const TrackInfo &a, const TrackInfo &b) {
        return a.display.toLower() < b.display.toLower();
    });

    // build playlist + list (indexes match)
    for (const auto &t : tracks) {
        if (pl)
            pl->addMedia(QUrl::fromLocalFile(t.file));

        auto *item = new QListWidgetItem(t.display);
        item->setData(Qt::UserRole + 1, t.artist);
        item->setData(Qt::UserRole + 2, t.title);
        item->setData(Qt::UserRole + 3, t.file);
        tracks_widget->addItem(item);
    }
}


