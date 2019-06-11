/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cstdint>

#include "edit_tool.h"
#include "grid_helper.h"
#include "pcb_actions.h"
#include "pcb_editor_control.h"
#include "pcbnew_control.h"
#include "pcbnew_picker_tool.h"
#include "selection_tool.h"
#include <3d_viewer/eda_3d_viewer.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <class_board.h>
#include <class_board_item.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <confirm.h>
#include <connectivity/connectivity_data.h>
#include <gal/graphics_abstraction_layer.h>
#include <io_mgr.h>
#include <kicad_clipboard.h>
#include <kicad_plugin.h>
#include <kiway.h>
#include <origin_viewitem.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_screen.h>
#include <pcbnew_id.h>
#include <properties.h>
#include <tool/tool_manager.h>
#include <view/view_controls.h>

#include <functional>
#include <footprint_viewer_frame.h>

using namespace std::placeholders;


// files.cpp
extern bool AskLoadBoardFileName( wxWindow* aParent, int* aCtl, wxString* aFileName,
                                    bool aKicadFilesOnly = false );
extern IO_MGR::PCB_FILE_T plugin_type( const wxString& aFileName, int aCtl );


// Display modes
TOOL_ACTION PCB_ACTIONS::showRatsnest( "pcbnew.Control.showRatsnest",
        AS_GLOBAL, 0, "",
        _( "Show Ratsnest" ), _( "Show board ratsnest" ),
        general_ratsnest_xpm );

TOOL_ACTION PCB_ACTIONS::ratsnestLineMode( "pcbnew.Control.ratsnestLineMode",
        AS_GLOBAL, 0, "",
        _( "Curved Ratsnest Lines" ), _( "Show ratsnest with curved lines" ),
        curved_ratsnest_xpm );

TOOL_ACTION PCB_ACTIONS::trackDisplayMode( "pcbnew.Control.trackDisplayMode",
        AS_GLOBAL, 
        'K', LEGACY_HK_NAME( "Track Display Mode" ),
        _( "Sketch Tracks" ), _( "Show tracks in outline mode" ),
        showtrack_xpm );

TOOL_ACTION PCB_ACTIONS::padDisplayMode( "pcbnew.Control.padDisplayMode",
        AS_GLOBAL, 0, "",
        _( "Sketch Pads" ), _( "Show pads in outline mode" ),
        pad_sketch_xpm );

TOOL_ACTION PCB_ACTIONS::viaDisplayMode( "pcbnew.Control.viaDisplayMode",
        AS_GLOBAL, 0, "",
        _( "Sketch Vias" ), _( "Show vias in outline mode" ),
        via_sketch_xpm );

TOOL_ACTION PCB_ACTIONS::graphicDisplayMode( "pcbnew.Control.graphicDisplayMode",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::moduleEdgeOutlines( "pcbnew.Control.graphicOutlines",
        AS_GLOBAL, 0, "",
        _( "Sketch Graphics" ), _( "Show footprint graphic items in outline mode" ),
        show_mod_edge_xpm );

TOOL_ACTION PCB_ACTIONS::zoneDisplayEnable( "pcbnew.Control.zoneDisplayEnable",
        AS_GLOBAL, 0, "",
        _( "Fill Zones" ), _( "Show filled areas of zones" ),
        show_zone_xpm);

TOOL_ACTION PCB_ACTIONS::zoneDisplayDisable( "pcbnew.Control.zoneDisplayDisable",
        AS_GLOBAL, 0, "",
        _( "Wireframe Zones" ), _( "Show only zone boundaries" ),
        show_zone_disable_xpm );

TOOL_ACTION PCB_ACTIONS::zoneDisplayOutlines( "pcbnew.Control.zoneDisplayOutlines",
        AS_GLOBAL, 0, "",
        _( "Sketch Zones" ), _( "Show solid areas of zones in outline mode" ),
        show_zone_outline_only_xpm);


// Layer control
TOOL_ACTION PCB_ACTIONS::layerTop( "pcbnew.Control.layerTop",
        AS_GLOBAL,
        WXK_PAGEUP, LEGACY_HK_NAME( "Switch to Component (F.Cu) layer" ),
        "Switch to Component (F.Cu) layer", "", 
        nullptr, AF_NONE, (void*) F_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner1( "pcbnew.Control.layerInner1",
        AS_GLOBAL, 
        WXK_F5, LEGACY_HK_NAME( "Switch to Inner layer 1" ),
        "Switch to Inner layer 1", "", 
        nullptr, AF_NONE, (void*) In1_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner2( "pcbnew.Control.layerInner2",
        AS_GLOBAL, 
        WXK_F6, LEGACY_HK_NAME( "Switch to Inner layer 2" ),
        "Switch to Inner layer 2", "", 
        nullptr, AF_NONE, (void*) In2_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner3( "pcbnew.Control.layerInner3",
        AS_GLOBAL, 
        WXK_F7, LEGACY_HK_NAME( "Switch to Inner layer 3" ),
        "Switch to Inner layer 3", "", 
        nullptr, AF_NONE, (void*) In3_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner4( "pcbnew.Control.layerInner4",
        AS_GLOBAL, 
        WXK_F8, LEGACY_HK_NAME( "Switch to Inner layer 4" ),
        "Switch to Inner layer 4", "", 
        nullptr, AF_NONE, (void*) In4_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner5( "pcbnew.Control.layerInner5",
        AS_GLOBAL, 
        MD_SHIFT + WXK_F5, LEGACY_HK_NAME( "Switch to Inner layer 5" ),
        "Switch to Inner layer 5", "", 
        nullptr, AF_NONE, (void*) In5_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner6( "pcbnew.Control.layerInner6",
        AS_GLOBAL, 
        MD_SHIFT + WXK_F6, LEGACY_HK_NAME( "Switch to Inner layer 6" ),
        "Switch to Inner layer 6", "", 
        nullptr, AF_NONE, (void*) In6_Cu );

TOOL_ACTION PCB_ACTIONS::layerBottom( "pcbnew.Control.layerBottom",
        AS_GLOBAL, 
        WXK_PAGEDOWN, LEGACY_HK_NAME( "Switch to Copper (B.Cu) layer" ),
        "Switch to Copper (B.Cu) layer", "", 
        nullptr, AF_NONE, (void*) B_Cu );

TOOL_ACTION PCB_ACTIONS::layerNext( "pcbnew.Control.layerNext",
        AS_GLOBAL, 
        '+', LEGACY_HK_NAME( "Switch to Next Layer" ),
        "Switch to Next Layer", "" );

TOOL_ACTION PCB_ACTIONS::layerPrev( "pcbnew.Control.layerPrev",
        AS_GLOBAL, 
        '-', LEGACY_HK_NAME( "Switch to Previous Layer" ),
        "Switch to Previous Layer", "" );

TOOL_ACTION PCB_ACTIONS::layerToggle( "pcbnew.Control.layerToggle",
        AS_GLOBAL, 
        'V', LEGACY_HK_NAME( "Add Through Via" ),
        "Add Through Via", "" );

TOOL_ACTION PCB_ACTIONS::layerAlphaInc( "pcbnew.Control.layerAlphaInc",
        AS_GLOBAL, 
        '}', LEGACY_HK_NAME( "Increment Layer Transparency (Modern Toolset only)" ),
        _( "Increase Layer Opacity" ), _( "Make the current layer more transparent" ),
        contrast_mode_xpm );

TOOL_ACTION PCB_ACTIONS::layerAlphaDec( "pcbnew.Control.layerAlphaDec",
        AS_GLOBAL, 
        '{', LEGACY_HK_NAME( "Decrement Layer Transparency (Modern Toolset only)" ),
        _( "Decrease Layer Opacity" ), _( "Make the current layer more transparent" ),
        contrast_mode_xpm );

TOOL_ACTION PCB_ACTIONS::layerChanged( "pcbnew.Control.layerChanged",
        AS_GLOBAL, 0, "",
        "", "",
        nullptr, AF_NOTIFY );

// Miscellaneous
TOOL_ACTION PCB_ACTIONS::selectionTool( "pcbnew.Control.selectionTool",
        AS_GLOBAL, 0, "",
        _( "Select item(s)" ), "",
        cursor_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::deleteTool( "pcbnew.Control.deleteTool",
        AS_GLOBAL, 0, "",
        _( "Delete Items Tool" ), _( "Click on items to delete them" ),
        delete_xpm );

PCBNEW_CONTROL::PCBNEW_CONTROL() :
    PCB_TOOL_BASE( "pcbnew.Control" ), 
    m_frame( nullptr )
{
    m_gridOrigin.reset( new KIGFX::ORIGIN_VIEWITEM() );
}


PCBNEW_CONTROL::~PCBNEW_CONTROL()
{
}


void PCBNEW_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH )
    {
        m_gridOrigin->SetPosition( board()->GetGridOrigin() );
        m_gridOrigin->SetColor( m_frame->GetGridColor() );
        getView()->Remove( m_gridOrigin.get() );
        getView()->Add( m_gridOrigin.get() );
    }
}


int PCBNEW_CONTROL::AddLibrary( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_PCB_MODULE_EDITOR ) || m_frame->IsType( FRAME_PCB ) )
    {
        if( aEvent.IsAction( &ACTIONS::newLibrary ) )
            static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->CreateNewLibrary();
        else if( aEvent.IsAction( &ACTIONS::addLibrary ) )
            static_cast<PCB_BASE_EDIT_FRAME*>( m_frame )->AddLibrary();
    }

    return 0;
}


int PCBNEW_CONTROL::Quit( const TOOL_EVENT& aEvent )
{
    m_frame->Close( false );
    return 0;
}


template<class T> void Flip( T& aValue )
{
    aValue = !aValue;
}

int PCBNEW_CONTROL::TrackDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts->m_DisplayPcbTrackFill );
    view()->UpdateDisplayOptions( opts );

    for( auto track : board()->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T )
            view()->Update( track, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ToggleRatsnest( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    if( aEvent.IsAction( &PCB_ACTIONS::showRatsnest ) )
    {
        PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) m_frame;
        frame->SetElementVisibility( LAYER_RATSNEST, !frame->IsElementVisible( LAYER_RATSNEST ) );
    }
    else if( aEvent.IsAction( &PCB_ACTIONS::ratsnestLineMode ) )
    {
        Flip( opts->m_DisplayRatsnestLinesCurved );
        view()->UpdateDisplayOptions( opts );
    }

    canvas()->RedrawRatsnest();
    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::PadDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts->m_DisplayPadFill );
    view()->UpdateDisplayOptions( opts );

    for( auto module : board()->Modules() ) // fixme: move to PCB_VIEW
    {
        for( auto pad : module->Pads() )
            view()->Update( pad, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ViaDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts->m_DisplayViaFill );
    view()->UpdateDisplayOptions( opts );

    for( auto track : board()->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_VIA_T )
            view()->Update( track, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::GraphicDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts->m_DisplayDrawItemsFill );
    view()->UpdateDisplayOptions( opts );

    for( auto item : board()->Drawings() )
    {
        view()->Update( item, KIGFX::GEOMETRY );
    }

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ModuleEdgeOutlines( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts->m_DisplayModEdgeFill );
    view()->UpdateDisplayOptions( opts );

    for( auto module : board()->Modules() )
    {
        for( auto item : module->GraphicalItems() )
        {
            if( item->Type() == PCB_MODULE_EDGE_T )
                view()->Update( item, KIGFX::GEOMETRY );
        }
    }

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ZoneDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    // Apply new display options to the GAL canvas
    if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayEnable ) )
        opts->m_DisplayZonesMode = 0;
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayDisable ) )
        opts->m_DisplayZonesMode = 1;
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayOutlines ) )
        opts->m_DisplayZonesMode = 2;
    else
        wxFAIL;

    view()->UpdateDisplayOptions( opts );

    for( int i = 0; i < board()->GetAreaCount(); ++i )
        view()->Update( board()->GetArea( i ), KIGFX::GEOMETRY );

    canvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::HighContrastMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts->m_ContrastModeDisplay );
    view()->UpdateDisplayOptions( opts );
    canvas()->SetHighContrastLayer( m_frame->GetActiveLayer() );

    return 0;
}


// Layer control
int PCBNEW_CONTROL::LayerSwitch( const TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, (PCB_LAYER_ID) aEvent.Parameter<intptr_t>() );

    return 0;
}


int PCBNEW_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    LAYER_NUM layer = editFrame->GetActiveLayer();

    if( layer < F_Cu || layer > B_Cu )
        return 0;

    int layerCount = board()->GetCopperLayerCount();

    if( layer == layerCount - 2 || layerCount < 2 )
        layer = B_Cu;
    else if( layer == B_Cu )
        layer = F_Cu;
    else
        ++layer;

    wxCHECK( IsCopperLayer( layer ), 0 );
    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );

    return 0;
}


int PCBNEW_CONTROL::LayerPrev( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    LAYER_NUM layer = editFrame->GetActiveLayer();

    if( layer < F_Cu || layer > B_Cu )
        return 0;

    int layerCount = board()->GetCopperLayerCount();

    if( layer == F_Cu || layerCount < 2 )
        layer = B_Cu;
    else if( layer == B_Cu )
        layer = layerCount - 2;
    else
        --layer;

    wxCHECK( IsCopperLayer( layer ), 0 );
    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );

    return 0;
}


int PCBNEW_CONTROL::LayerToggle( const TOOL_EVENT& aEvent )
{
    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    PCB_SCREEN* screen = m_frame->GetScreen();

    if( currentLayer == screen->m_Route_Layer_TOP )
        m_frame->SwitchLayer( NULL, screen->m_Route_Layer_BOTTOM );
    else
        m_frame->SwitchLayer( NULL, screen->m_Route_Layer_TOP );

    return 0;
}


// It'd be nice to share the min/max with the DIALOG_COLOR_PICKER, but those are
// set in wxFormBuilder.
#define ALPHA_MIN 0.20
#define ALPHA_MAX 1.00
#define ALPHA_STEP 0.05

int PCBNEW_CONTROL::LayerAlphaInc( const TOOL_EVENT& aEvent )
{
    auto& settings = m_frame->Settings().Colors();

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings.GetLayerColor( currentLayer );

    if( currentColor.a <= ALPHA_MAX - ALPHA_STEP )
    {
        currentColor.a += ALPHA_STEP;
        settings.SetLayerColor( currentLayer, currentColor );

        KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
        view->GetPainter()->GetSettings()->ImportLegacyColors( &settings );
        view->UpdateLayerColor( currentLayer );

        wxUpdateUIEvent dummy;
        static_cast<PCB_EDIT_FRAME*>( m_frame )->OnUpdateLayerAlpha( dummy );
    }
    else
        wxBell();

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaDec( const TOOL_EVENT& aEvent )
{
    auto& settings = m_frame->Settings().Colors();

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings.GetLayerColor( currentLayer );

    if( currentColor.a >= ALPHA_MIN + ALPHA_STEP )
    {
        currentColor.a -= ALPHA_STEP;
        settings.SetLayerColor( currentLayer, currentColor );

        KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
        view->GetPainter()->GetSettings()->ImportLegacyColors( &settings );
        view->UpdateLayerColor( currentLayer );

        wxUpdateUIEvent dummy;
        static_cast<PCB_BASE_FRAME*>( m_frame )->OnUpdateLayerAlpha( dummy );
    }
    else
        wxBell();

    return 0;
}


// Grid control
int PCBNEW_CONTROL::GridFast1( const TOOL_EVENT& aEvent )
{
    m_frame->SetFastGrid1();
    updateGrid();

    return 0;
}


int PCBNEW_CONTROL::GridFast2( const TOOL_EVENT& aEvent )
{
    m_frame->SetFastGrid2();
    updateGrid();

    return 0;
}


bool PCBNEW_CONTROL::DoSetGridOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                      BOARD_ITEM* originViewItem, const VECTOR2D& aPoint )
{
    aFrame->SetGridOrigin( wxPoint( aPoint.x, aPoint.y ) );
    aView->GetGAL()->SetGridOrigin( aPoint );
    originViewItem->SetPosition( wxPoint( aPoint.x, aPoint.y ) );
    aView->MarkDirty();
    aFrame->OnModify();
    return true;
}


bool PCBNEW_CONTROL::SetGridOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                    BOARD_ITEM* originViewItem, const VECTOR2D& aPoint )
{
    aFrame->SaveCopyInUndoList( originViewItem, UR_GRIDORIGIN );
    return DoSetGridOrigin( aView, aFrame, originViewItem, aPoint );
}


int PCBNEW_CONTROL::GridSetOrigin( const TOOL_EVENT& aEvent )
{
    VECTOR2D* origin = aEvent.Parameter<VECTOR2D*>();

    if( origin )
    {
        // We can't undo the other grid dialog settings, so no sense undoing just the origin

        DoSetGridOrigin( getView(), m_frame, m_gridOrigin.get(), *origin );
        delete origin;
    }
    else
    {
        Activate();

        PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
        wxCHECK( picker, 0 );

        // TODO it will not check the toolbar button in module editor, as it uses a different ID..
        m_frame->SetToolID( ID_PCB_PLACE_GRID_COORD_BUTT, wxCURSOR_PENCIL, _( "Adjust grid origin" ) );
        picker->SetClickHandler( std::bind( SetGridOrigin, getView(), m_frame, m_gridOrigin.get(), _1 ) );
        picker->Activate();
        Wait();
    }

    return 0;
}


int PCBNEW_CONTROL::GridResetOrigin( const TOOL_EVENT& aEvent )
{
    SetGridOrigin( getView(), m_frame, m_gridOrigin.get(), VECTOR2D( 0, 0 ) );

    return 0;
}


// Miscellaneous
static bool deleteItem( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SELECTION_TOOL* selectionTool = aToolMgr->GetTool<SELECTION_TOOL>();
    wxCHECK( selectionTool, false );

    aToolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    const PCBNEW_SELECTION& selection = selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED ); } );

    if( selection.Empty() )
        return true;

    aToolMgr->RunAction( PCB_ACTIONS::remove, true );

    return true;
}


int PCBNEW_CONTROL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    Activate();

    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    wxCHECK( picker, 0 );

    m_frame->SetToolID( m_editModules ? ID_MODEDIT_DELETE_TOOL : ID_PCB_DELETE_ITEM_BUTT,
            wxCURSOR_BULLSEYE, _( "Delete item" ) );
    picker->SetClickHandler( std::bind( deleteItem, m_toolMgr, _1 ) );
    picker->Activate();
    Wait();

    return 0;
}


int PCBNEW_CONTROL::Paste( const TOOL_EVENT& aEvent )
{
    CLIPBOARD_IO pi;
    BOARD_ITEM* clipItem = pi.Parse();

    if( !clipItem )
        return 0;

    if( clipItem->Type() == PCB_T )
        static_cast<BOARD*>( clipItem )->ClearAllNetCodes();

    bool editModules = m_editModules || frame()->IsType( FRAME_PCB_MODULE_EDITOR );

    // The clipboard can contain two different things, an entire kicad_pcb
    // or a single module

    if( editModules && ( !board() || !module() ) )
    {
        wxLogDebug( wxT( "Attempting to paste to empty module editor window\n") );
        return 0;
    }

    switch( clipItem->Type() )
    {
        case PCB_T:
        {
            if( editModules )
            {
                wxLogDebug( wxT( "attempting to paste a pcb in the footprint editor\n") );
                return 0;
            }

            placeBoardItems( static_cast<BOARD*>( clipItem ) );
            break;
        }

        case PCB_MODULE_T:
        {
            std::vector<BOARD_ITEM*> items;

            clipItem->SetParent( board() );

            if( editModules )
            {
                auto oldModule = static_cast<MODULE*>( clipItem );
                auto newModule = board()->GetFirstModule();

                for( auto pad : oldModule->Pads() )
                {
                    pad->SetParent( newModule );
                    items.push_back( pad );
                }

                oldModule->Pads().clear();

                for( auto item : oldModule->GraphicalItems() )
                {
                    item->SetParent( newModule );
                    items.push_back( item );
                }

                oldModule->GraphicalItems().clear();
            }
            else
            {
                items.push_back( clipItem );
            }

            placeBoardItems( items, true );
            break;
        }

        default:
            m_frame->DisplayToolMsg( _( "Invalid clipboard contents" ) );
            break;
    }

    return 1;
}


int PCBNEW_CONTROL::AppendBoardFromFile( const TOOL_EVENT& aEvent )
{
    int open_ctl;
    wxString fileName;

    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    // Pick a file to append
    if( !AskLoadBoardFileName( editFrame, &open_ctl, &fileName, true ) )
        return 1;

    IO_MGR::PCB_FILE_T pluginType = plugin_type( fileName, open_ctl );
    PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

    return AppendBoard( *pi, fileName );
}


// Helper function for PCBNEW_CONTROL::placeBoardItems()
template<typename T>
static void moveNoFlagToVector( std::deque<T>& aList, std::vector<BOARD_ITEM*>& aTarget, bool aIsNew )
{
    std::copy_if( aList.begin(), aList.end(), std::back_inserter( aTarget ),
            [](T aItem){
                bool retval = ( aItem->GetFlags() & FLAG0 ) == 0;
                aItem->ClearFlags( FLAG0 );
                return retval;
            } );

    if( aIsNew )
        aList.clear();
}


static void moveNoFlagToVector(  ZONE_CONTAINERS& aList, std::vector<BOARD_ITEM*>& aTarget, bool aIsNew )
{
    if( aList.size() == 0 )
        return;

    auto obj = aList.front();
    int idx = 0;

    if( aIsNew )
    {
        obj = aList.back();
        aList.pop_back();
    }

    for( ; obj ; )
    {
        if( obj->GetFlags() & FLAG0 )
            obj->ClearFlags( FLAG0 );
        else
            aTarget.push_back( obj );

        if( aIsNew )
        {
            if( aList.size() )
            {
                obj = aList.back();
                aList.pop_back();
            }
            else
                obj = nullptr;
        }
        else
            obj = idx < int(aList.size()-1) ? aList[++idx] : nullptr;
    }
}



int PCBNEW_CONTROL::placeBoardItems( BOARD* aBoard )
{
    // items are new if the current board is not the board source
    bool isNew = board() != aBoard;
    std::vector<BOARD_ITEM*> items;

    moveNoFlagToVector( aBoard->Tracks(), items, isNew );
    moveNoFlagToVector( aBoard->Modules(), items, isNew );
    moveNoFlagToVector( aBoard->Drawings(), items, isNew );
    moveNoFlagToVector( aBoard->Zones(), items, isNew );

    return placeBoardItems( items, isNew );
}


int PCBNEW_CONTROL::placeBoardItems( std::vector<BOARD_ITEM*>& aItems, bool aIsNew )
{
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    auto selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    auto editTool = m_toolMgr->GetTool<EDIT_TOOL>();

    PCBNEW_SELECTION& selection = selectionTool->GetSelection();

    for( auto item : aItems )
    {
        item->SetSelected();
        selection.Add( item );

        // Add or just select items for the move/place command
        if( aIsNew )
            editTool->GetCurrentCommit()->Add( item );
        else
            editTool->GetCurrentCommit()->Added( item );
    }

    selection.SetReferencePoint( VECTOR2I( 0, 0 ) );

    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );
    m_toolMgr->RunAction( PCB_ACTIONS::move, true );

    return 0;
}


int PCBNEW_CONTROL::AppendBoard( PLUGIN& pi, wxString& fileName )
{
    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 1;

    BOARD* brd = board();

    if( !brd )
        return 1;

    // Mark existing items, in order to know what are the new items so we can select only
    // the new items after loading
    for( auto track : brd->Tracks() )
        track->SetFlags( FLAG0 );

    for( auto module : brd->Modules() )
        module->SetFlags( FLAG0 );

    for( auto drawing : brd->Drawings() )
        drawing->SetFlags( FLAG0 );

    for( auto zone : brd->Zones() )
        zone->SetFlags( FLAG0 );

    // Keep also the count of copper layers, to adjust if necessary
    int initialCopperLayerCount = brd->GetCopperLayerCount();
    LSET initialEnabledLayers = brd->GetEnabledLayers();

    // Load the data
    try
    {
        PROPERTIES  props;
        char        xbuf[30];
        char        ybuf[30];

        // EAGLE_PLUGIN can use this info to center the BOARD, but it does not yet.
        sprintf( xbuf, "%d", editFrame->GetPageSizeIU().x );
        sprintf( ybuf, "%d", editFrame->GetPageSizeIU().y );

        props["page_width"]  = xbuf;
        props["page_height"] = ybuf;

        editFrame->GetDesignSettings().m_NetClasses.Clear();
        pi.Load( fileName, brd, &props );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ), GetChars( ioe.What() ));
        DisplayError( editFrame, msg );

        return 0;
    }

    // rebuild nets and ratsnest before any use of nets
    brd->BuildListOfNets();
    brd->SynchronizeNetsAndNetClasses();
    brd->BuildConnectivity();

    // Synchronize layers
    // we should not ask PLUGINs to do these items:
    int copperLayerCount = brd->GetCopperLayerCount();

    if( copperLayerCount > initialCopperLayerCount )
        brd->SetCopperLayerCount( copperLayerCount );

    // Enable all used layers, and make them visible:
    LSET enabledLayers = brd->GetEnabledLayers();
    enabledLayers |= initialEnabledLayers;
    brd->SetEnabledLayers( enabledLayers );
    brd->SetVisibleLayers( enabledLayers );

    return placeBoardItems( brd );
}


int PCBNEW_CONTROL::Undo( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = dynamic_cast<PCB_BASE_EDIT_FRAME*>( m_frame );
    wxCommandEvent       dummy;

    if( editFrame )
        editFrame->RestoreCopyFromUndoList( dummy );

    return 0;
}


int PCBNEW_CONTROL::Redo( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = dynamic_cast<PCB_BASE_EDIT_FRAME*>( m_frame );
    wxCommandEvent       dummy;

    if( editFrame )
        editFrame->RestoreCopyFromRedoList( dummy );
    return 0;
}


int PCBNEW_CONTROL::Show3DViewer( const TOOL_EVENT& aEvent )
{
    EDA_3D_VIEWER* draw3DFrame = m_frame->CreateAndShow3D_Frame();

    // Suppress warnings on non-Mac systems
    [&draw3DFrame] {}();

    if( m_frame->IsType( FRAME_PCB_MODULE_VIEWER )
     || m_frame->IsType( FRAME_PCB_MODULE_VIEWER_MODAL )
     || m_frame->IsType( FRAME_PCB_FOOTPRINT_WIZARD ) )
    {
        m_frame->Update3DView( true );

#ifdef  __WXMAC__
        // A stronger version of Raise() which promotes the window to its parent's level.
        draw3DFrame->ReparentQuasiModal();
#endif
    }

    return 0;
}


void PCBNEW_CONTROL::updateGrid()
{
    BASE_SCREEN* screen = m_frame->GetScreen();
    //GRID_TYPE grid = screen->GetGrid( idx );
    getView()->GetGAL()->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}


int PCBNEW_CONTROL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    SELECTION_TOOL*   selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    PCBNEW_SELECTION& selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM*       item = (EDA_ITEM*) selection.Front();
        MSG_PANEL_ITEMS msgItems;

        item->GetMsgPanelInfo( m_frame->GetUserUnits(), msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else if( selection.GetSize() > 1 )
    {
        MSG_PANEL_ITEMS msgItems;
        wxString        msg = wxString::Format( wxT( "%d" ), selection.GetSize() );

        msgItems.push_back( MSG_PANEL_ITEM( _( "Selected Items" ), msg, DARKCYAN ) );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->SetMsgPanel( m_frame->GetBoard() );
    }

    return 0;
}


void PCBNEW_CONTROL::setTransitions()
{
    Go( &PCBNEW_CONTROL::AddLibrary,           ACTIONS::newLibrary.MakeEvent() );
    Go( &PCBNEW_CONTROL::AddLibrary,           ACTIONS::addLibrary.MakeEvent() );
    Go( &PCBNEW_CONTROL::Print,                ACTIONS::print.MakeEvent() );
    Go( &PCBNEW_CONTROL::Quit,                 ACTIONS::quit.MakeEvent() );
    
    // Display modes
    Go( &PCBNEW_CONTROL::TrackDisplayMode,     PCB_ACTIONS::trackDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ToggleRatsnest,       PCB_ACTIONS::showRatsnest.MakeEvent() );
    Go( &PCBNEW_CONTROL::ToggleRatsnest,       PCB_ACTIONS::ratsnestLineMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::PadDisplayMode,       PCB_ACTIONS::padDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ViaDisplayMode,       PCB_ACTIONS::viaDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::GraphicDisplayMode,   PCB_ACTIONS::graphicDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ModuleEdgeOutlines,   PCB_ACTIONS::moduleEdgeOutlines.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,      PCB_ACTIONS::zoneDisplayEnable.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,      PCB_ACTIONS::zoneDisplayDisable.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,      PCB_ACTIONS::zoneDisplayOutlines.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastMode,     ACTIONS::highContrastMode.MakeEvent() );

    // Layer control
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerTop.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner1.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner2.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner3.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner4.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner5.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerInner6.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,          PCB_ACTIONS::layerBottom.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerNext,            PCB_ACTIONS::layerNext.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerPrev,            PCB_ACTIONS::layerPrev.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerToggle,          PCB_ACTIONS::layerToggle.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaInc,        PCB_ACTIONS::layerAlphaInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaDec,        PCB_ACTIONS::layerAlphaDec.MakeEvent() );

    // Grid control
    Go( &PCBNEW_CONTROL::GridFast1,            ACTIONS::gridFast1.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridFast2,            ACTIONS::gridFast2.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridSetOrigin,        ACTIONS::gridSetOrigin.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridResetOrigin,      ACTIONS::gridResetOrigin.MakeEvent() );

    Go( &PCBNEW_CONTROL::Undo,                 ACTIONS::undo.MakeEvent() );
    Go( &PCBNEW_CONTROL::Redo,                 ACTIONS::redo.MakeEvent() );

    // Miscellaneous
    Go( &PCBNEW_CONTROL::DeleteItemCursor,     PCB_ACTIONS::deleteTool.MakeEvent() );
    Go( &PCBNEW_CONTROL::Show3DViewer,         ACTIONS::show3DViewer.MakeEvent() );

    // Append control
    Go( &PCBNEW_CONTROL::AppendBoardFromFile,  PCB_ACTIONS::appendBoard.MakeEvent() );

    Go( &PCBNEW_CONTROL::Paste,                ACTIONS::paste.MakeEvent() );

    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::SelectedEvent );
    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::UnselectedEvent );
    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::ClearedEvent );
    Go( &PCBNEW_CONTROL::UpdateMessagePanel,   EVENTS::SelectedItemsModified );
}


