#ifndef WAYPOINTITEM_H
#define WAYPOINTITEM_H

#include "map_item.h"
#include <QBrush>
#include "graphics_point.h"
#include "graphics_text.h"
#include "waypoint.h"
#include <memory>

class WaypointItem : public MapItem
{
        Q_OBJECT
public:
    WaypointItem(Point2DLatLon pt, QString ac_id, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    WaypointItem(Waypoint* wp, QString ac_id, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    // return position of the underlying "real" waypoint
    Point2DLatLon position() {return Point2DLatLon(original_waypoint);}
    Waypoint* getOriginalWaypoint() {return original_waypoint;}
    Waypoint* waypoint() {return _waypoint;}
    void setPosition(Point2DLatLon ll);
    QPointF scenePos();
    virtual void addToMap(MapWidget* map);
    virtual void setHighlighted(bool h);
    virtual void updateZValue();
    virtual void setForbidHighlight(bool fh);
    virtual void setEditable(bool ed);
    virtual void removeFromScene(MapWidget* map);
    virtual void updateGraphics(MapWidget* map);
    void update();

    // set original_waypoint position to the current _waypoint position.
    void commitPosition();

    virtual ItemType getType() {return ITEM_WAYPOINT;}
    void setIgnoreEvent(bool ignore);
    bool isMoving() {return moving;}
    void setStyle(GraphicsPoint::Style s);
    void setAnimation(GraphicsObject::Animation a);
    void setMoving(bool m){moving = m;}
    void setAnimate(bool animate);

signals:
    void waypointMoved(Point2DLatLon latlon_pos);
    void waypointMoveFinished();
    void waypointDoubleClicked();


private:
    void init();
    GraphicsPoint * point;
    GraphicsText* graphics_text;
    Waypoint* original_waypoint;
    // waypoint used to draw this WaypointItem
    Waypoint* _waypoint;
    int altitude;
    bool moving;
};

#endif // WAYPOINTITEM_H
