#include <modules/skybrowser/include/renderableskybrowser.h>
#include <modules/webbrowser/webbrowsermodule.h>
#include <modules/webbrowser/include/webkeyboardhandler.h>
#include <modules/webbrowser/include/browserinstance.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/engine/windowdelegate.h>
#include <openspace/engine/moduleengine.h>
#include <openspace/engine/globals.h>
#include <ghoul/misc/dictionaryjsonformatter.h> // formatJson
#include <ghoul/misc/profiling.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/textureunit.h>
#include <ghoul/opengl/texture.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/logging/logmanager.h>


namespace {

    constexpr const char* _loggerCat = "RenderableSkyBrowser";

    const openspace::properties::Property::PropertyInfo DimensionsInfo = {
        "Dimensions",
        "Browser Dimensions",
        "Set the dimensions of the web browser windows."
    };
    const openspace::properties::Property::PropertyInfo UrlInfo = {
        "Url",
        "URL",
        "The URL to load"
    };

    const openspace::properties::Property::PropertyInfo ReloadInfo = {
        "Reload",
        "Reload",
        "Reload the web browser"
    };

    struct [[codegen::Dictionary(ScreenSpaceSkyBrowser)]] Parameters {

        // [[codegen::verbatim(DimensionsInfo.description)]]
        std::optional<glm::vec2> browserDimensions;
    };

#include "renderableskybrowser_codegen.cpp"
} // namespace

namespace openspace {

    void RenderableSkyBrowser::ScreenSpaceRenderHandler::draw() {}

    void RenderableSkyBrowser::ScreenSpaceRenderHandler::render() {}

    void RenderableSkyBrowser::ScreenSpaceRenderHandler::setTexture(GLuint t) {
        _texture = t;
    }


    RenderableSkyBrowser::RenderableSkyBrowser(const ghoul::Dictionary& dictionary)
        : RenderablePlane(dictionary)
        , _url(UrlInfo)
        , _dimensions(DimensionsInfo, glm::vec2(0.f), glm::vec2(0.f), glm::vec2(3000.f))
        , _reload(ReloadInfo)
    {

        std::string identifier;
        if (dictionary.hasValue<std::string>(KeyIdentifier)) {
            identifier = dictionary.value<std::string>(KeyIdentifier);
        }
        else {
            identifier = "RenderableSkyBrowser";
        }
        setIdentifier(identifier);

        if (dictionary.hasValue<std::string>(UrlInfo.identifier)) {
            _url = dictionary.value<std::string>(UrlInfo.identifier);
        }

        // Ensure the texture is a square for now
        // Maybe change later
        glm::vec2 windowDimensions = global::windowDelegate->currentSubwindowSize();
        float maxDimension = std::max(windowDimensions.x, windowDimensions.y);
        _dimensions = { maxDimension, maxDimension };

        // Create browser and render handler
        _renderHandler = new ScreenSpaceRenderHandler();
        _keyboardHandler = new WebKeyboardHandler();
        _browserInstance = std::make_unique<BrowserInstance>(
            _renderHandler,
            _keyboardHandler
            );

        _url.onChange([this]() { _isUrlDirty = true; });
        _dimensions.onChange([this]() { _isDimensionsDirty = true; });
        _reload.onChange([this]() { _browserInstance->reloadBrowser(); });

        addProperty(_url);
        addProperty(_dimensions);
        addProperty(_reload);

        WebBrowserModule* webBrowser = global::moduleEngine->module<WebBrowserModule>();
        if (webBrowser) {
            webBrowser->addBrowser(_browserInstance.get());
        }
    }

    void RenderableSkyBrowser::initializeGL() {
        RenderablePlane::initializeGL();
        _texture = std::make_unique<ghoul::opengl::Texture>(
            glm::uvec3(_dimensions.value(), 1.0f)
            );

        _renderHandler->setTexture(*_texture);

        _browserInstance->initialize();
        _browserInstance->loadUrl(_url);
        _dimensions = _texture->dimensions();
    }

    void RenderableSkyBrowser::deinitializeGL() {
        RenderablePlane::deinitializeGL();
        _renderHandler->setTexture(0);
        _texture = nullptr;

        std::string urlString;
        _url.getStringValue(urlString);
        LDEBUG(fmt::format("Deinitializing RenderableSkyBrowser: {}", urlString));

        _browserInstance->close(true);

        WebBrowserModule* webBrowser = global::moduleEngine->module<WebBrowserModule>();
        if (webBrowser) {
            webBrowser->removeBrowser(_browserInstance.get());
            _browserInstance.reset();
        }
        else {
            LWARNING("Could not find WebBrowserModule");
        }

        RenderablePlane::deinitializeGL();
    }

    void RenderableSkyBrowser::update(const UpdateData& data) {
        RenderablePlane::update(data);
        _renderHandler->updateTexture();
        _objectSize = _texture->dimensions();

        if (_isUrlDirty) {
            _browserInstance->loadUrl(_url);
            _isUrlDirty = false;
        }

        if (_isDimensionsDirty) {
            _browserInstance->reshape(_dimensions.value());
            _isDimensionsDirty = false;
        }
    }

    void RenderableSkyBrowser::bindTexture() {
        if (_texture) {
            _texture->bind();
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void RenderableSkyBrowser::executeJavascript(std::string script) const {
        //LINFOC(_loggerCat, "Executing javascript " + script);
        if (_browserInstance && _browserInstance->getBrowser() && _browserInstance->getBrowser()->GetMainFrame()) {
            CefRefPtr<CefFrame> frame = _browserInstance->getBrowser()->GetMainFrame();
            frame->ExecuteJavaScript(script, frame->GetURL(), 0);
        }
    }

    bool RenderableSkyBrowser::sendMessageToWWT(const ghoul::Dictionary& msg) {
        std::string script = "sendMessageToWWT(" + ghoul::formatJson(msg) + ");";
        executeJavascript(script);
        return true;
    }
} // namespace
