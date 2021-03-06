/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014- Heikki Pulkkinen.
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

#include"trackitems.h"

#include <class_marker_pcb.h>
#include <view/view.h>
#ifdef NEWCONALGO
#include <connectivity_data.h>
#endif

using namespace TrackNodeItem;
using namespace TrackItems;
using namespace TrackNodeItems;


//----------------------------------------------------------------------------------------
// Track items.
//----------------------------------------------------------------------------------------
const wxString TRACKITEMS::TXT_TRACKITEMS = _( "Tracks Tools" );

TRACKITEMS::TRACKITEMS( const BOARD* aBoard )
{
    m_Board = const_cast<BOARD*>( aBoard );
    m_Teardrops = new TEARDROPS( this, aBoard );
    m_RoundedTracksCorners = new ROUNDED_TRACKS_CORNERS( this, aBoard );
    m_NetCodeFirstTrackItem = new NETCODE_FIRST_TRACKITEM( aBoard );
}

TRACKITEMS::~TRACKITEMS()
{
    delete m_Teardrops;
    m_Teardrops = nullptr;
    delete m_RoundedTracksCorners;
    m_RoundedTracksCorners = nullptr;
    delete m_NetCodeFirstTrackItem;
    m_NetCodeFirstTrackItem = nullptr;
}


#ifdef NEWCONALGO
void TRACKITEMS::SetEditFrame( const PCB_EDIT_FRAME* aEditFrame )
#else
void TRACKITEMS::SetEditFrame( const PCB_EDIT_FRAME* aEditFrame, wxMenu* aMenu )
#endif
{
    m_EditFrame = const_cast<PCB_EDIT_FRAME*>( aEditFrame );
#ifdef NEWCONALGO
    m_Teardrops->SetEditFrame( aEditFrame );
    m_RoundedTracksCorners->SetEditFrame( aEditFrame );
}

void TRACKITEMS::SetMenu( wxMenu* aMenu )
{
#endif //Yes it is up here.
    if( aMenu )
    {
        int item_pos = aMenu->GetMenuItemCount();
        while( --item_pos >= 0 )
        {
            wxMenuItem* item = aMenu->FindItemByPosition( item_pos );
            aMenu->Destroy( item );
        }

#ifdef NEWCONALGO
        if( m_Teardrops )
        {
            m_Teardrops->SetMenu( aMenu );
            AddMenuItem( aMenu,
                            m_Teardrops->GetMenu(), ID_POPUP_PCB_TEARDROPS_COMMON_MNU,
                            TEARDROPS::TXT_TEARDROPS,
                            KiBitmap( pad_xpm ) );
        }

        if( m_RoundedTracksCorners )
        {
            m_RoundedTracksCorners->SetMenu( aMenu );
            AddMenuItem( aMenu,
                        m_RoundedTracksCorners->GetMenu(),
                        ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_COMMON_MNU,
                        ROUNDED_TRACKS_CORNERS::TXT_ROUNDEDTRACKSCORNERS,
                        KiBitmap( add_tracks_xpm ) );
        }
#else
        m_Teardrops->SetEditFrame( aEditFrame );
        m_RoundedTracksCorners->SetEditFrame( aEditFrame );

        AddMenuItem( aMenu,
                     m_Teardrops->GetMenu(), ID_POPUP_PCB_TEARDROPS_COMMON_MNU,
                     TEARDROPS::TXT_TEARDROPS,
                     KiBitmap( pad_xpm ) );

        AddMenuItem( aMenu,
                     m_RoundedTracksCorners->GetMenu(),
                     ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_COMMON_MNU,
                     ROUNDED_TRACKS_CORNERS::TXT_ROUNDEDTRACKSCORNERS,
                     KiBitmap( add_tracks_xpm ) );

#endif
        wxMenu* misc_menu = new wxMenu;
        AddMenuItem( misc_menu,
                     ID_POPUP_PCB_TRACKS_MARK_SHARP_ANGLES,
                     _( "Mark Sharp Angles of Tracks" ),
                     KiBitmap( add_tracks_xpm ) );
        AddMenuItem( misc_menu,
                     ID_POPUP_PCB_TRACKS_CONNECT_CENTER_IN_ITEM,
                     _( "Centralize Pad / Via Connections of Tracks" ),
                     KiBitmap( via_xpm ) );
        AddMenuItem( aMenu,
                     misc_menu,
                     ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_COMMON_MNU,
                     _( "Misc" ),
                     KiBitmap( options_all_tracks_and_vias_xpm ) );
    }
}

//---------------------------------------------------------------------------------------------------
// Get pads and vias
//---------------------------------------------------------------------------------------------------
TRACKITEMS::NET_SCAN_GET_VIA::NET_SCAN_GET_VIA( const TRACK* aStartTrack,
                                                const wxPoint aPos,
                                                const TRACKITEMS* aParent
                                              ) :
    NET_SCAN_BASE( aStartTrack, aParent )
{
    m_pos = aPos;
    m_result_via = nullptr;
}

bool TRACKITEMS::NET_SCAN_GET_VIA::ExecuteAt( TRACK* aTrack )
{
    if( aTrack->Type() == PCB_VIA_T )
        if( aTrack->IsOnLayer( m_scan_start_track->GetLayer() ) )
            if( aTrack->GetStart() == m_pos )
            {
                m_result_via = static_cast<VIA*>( const_cast<TRACK*>( aTrack ) );
                return true;
            }
    return false;
}

VIA* TRACKITEMS::GetVia( const TRACK* aTrackSegAt, const wxPoint aPosAt ) const
{
    VIA* result = nullptr;
    if( aTrackSegAt &&
        ( ( aTrackSegAt->GetStart() == aPosAt ) ||
        ( aTrackSegAt->GetEnd() == aPosAt ) ) )
    {
        std::unique_ptr<NET_SCAN_GET_VIA> via( new NET_SCAN_GET_VIA( aTrackSegAt, aPosAt, this ) );
        if( via )
        {
            via->Execute();
            result = via->GetResult();
        }
    }
    return result;
}

TRACKITEMS::NET_SCAN_GET_ENDPOS_VIA::NET_SCAN_GET_ENDPOS_VIA( const TRACK* aStartTrack,
                                                          const TRACKITEMS* aParent
                                                        ) :
    NET_SCAN_GET_VIA( aStartTrack, wxPoint{0,0}, aParent )
{
}

bool TRACKITEMS::NET_SCAN_GET_ENDPOS_VIA::ExecuteAt( TRACK* aTrack )
{
    if( aTrack->Type() == PCB_VIA_T )
        if( aTrack->IsOnLayer( m_scan_start_track->GetLayer() ) )
            if( aTrack->GetStart() == m_scan_start_track->GetEnd() )
            {
                m_result_via = static_cast<VIA*>( const_cast<TRACK*>( aTrack ) );
                return true;
            }
    return false;
}

VIA* TRACKITEMS::EndPosVia( const TRACK* aTrackSegAt ) const
{
    VIA* result = nullptr;
    if( aTrackSegAt )
    {
        std::unique_ptr<NET_SCAN_GET_ENDPOS_VIA> via(
            new NET_SCAN_GET_ENDPOS_VIA( aTrackSegAt, this ) );
        if( via )
        {
            via->Execute();
            result = via->GetResult();
        }
    }
    return result;
}

TRACKITEMS::NET_SCAN_GET_STARTPOS_VIA::NET_SCAN_GET_STARTPOS_VIA( const TRACK* aStartTrack,
                                                          const TRACKITEMS* aParent
                                                        ) :
    NET_SCAN_GET_VIA( aStartTrack, wxPoint{0,0}, aParent )
{
    m_reverse = true;
}

bool TRACKITEMS::NET_SCAN_GET_STARTPOS_VIA::ExecuteAt( TRACK* aTrack )
{
    if( aTrack->Type() == PCB_VIA_T )
        if( aTrack->IsOnLayer( m_scan_start_track->GetLayer() ) )
            if( aTrack->GetStart() == m_scan_start_track->GetStart() )
            {
                m_result_via = static_cast<VIA*>( const_cast<TRACK*>( aTrack ) );
                return true;
            }
    return false;
}

VIA* TRACKITEMS::StartPosVia( const TRACK* aTrackSegAt ) const
{
    VIA* result = nullptr;
    if( aTrackSegAt )
    {
        std::unique_ptr<NET_SCAN_GET_STARTPOS_VIA> via( new NET_SCAN_GET_STARTPOS_VIA( aTrackSegAt, this ) );
        if( via )
        {
            via->Execute();
            result = via->GetResult();
        }
    }
    return result;
}

D_PAD* TRACKITEMS::GetPad( const TRACK* aTrackSegAt, const wxPoint aPosAt ) const
{
    if( aTrackSegAt &&
        ( ( aTrackSegAt->GetStart() == aPosAt ) ||
        ( aTrackSegAt->GetEnd() == aPosAt ) ) )
    {
        if( aTrackSegAt->Type() == PCB_TRACE_T )
        {
            std::vector<D_PAD*> pads = GetPads( aTrackSegAt->GetNetCode() );
            for( auto pad : pads )
                if( pad->GetPosition() == aPosAt )
                    return pad;
        }
    }
    return nullptr;
}

D_PAD* TRACKITEMS::EndPosPad( const TRACK* aTrackSegAt ) const
{
    if( aTrackSegAt )
    {
        if( aTrackSegAt->Type() == PCB_TRACE_T )
        {
            std::vector<D_PAD*> pads = GetPads( aTrackSegAt->GetNetCode() );
            for( auto pad : pads )
                if( aTrackSegAt->GetEnd() == pad->GetPosition() )
                    return pad;
        }
    }
    return nullptr;
}

D_PAD* TRACKITEMS::StartPosPad( const TRACK* aTrackSegAt ) const
{
    if( aTrackSegAt )
    {
        if( aTrackSegAt->Type() == PCB_TRACE_T )
        {
            std::vector<D_PAD*> pads = GetPads( aTrackSegAt->GetNetCode() );
            for( auto pad : pads )
                if( aTrackSegAt->GetStart() == pad->GetPosition() )
                    return pad;
        }
    }
    return nullptr;
}

// Get all pads at aNetCode.
TRACKITEMS::PADS_SCAN_GET_PADS_IN_NET::PADS_SCAN_GET_PADS_IN_NET( const int aNetCode,
                                                                  std::vector<D_PAD*>& aPadsList,
                                                                  const BOARD* aBoard
                                                                ) :
    PADS_SCAN_BASE( aBoard )
{
    m_pads_list = &aPadsList;
    m_netcode = aNetCode;
}

bool TRACKITEMS::PADS_SCAN_GET_PADS_IN_NET::ExecutePad( const D_PAD* aPad )
{
    if( aPad->GetNetCode() == m_netcode )
        m_pads_list->push_back( const_cast<D_PAD*>( aPad ) );
    return false;
}

std::vector<D_PAD*> TRACKITEMS::GetPads( const int aNetCode ) const
{
    std::vector<D_PAD*> pads_list;
    pads_list.clear();
    std::unique_ptr<PADS_SCAN_GET_PADS_IN_NET> get_pads(
        new PADS_SCAN_GET_PADS_IN_NET( aNetCode, pads_list, m_Board ) );

    if( get_pads )
        get_pads->Execute();
    return pads_list;
}

//---------------------------------------------------------------------------------------------------
// PADs scan base.
//---------------------------------------------------------------------------------------------------
TrackItems::PADS_SCAN_BASE::PADS_SCAN_BASE( const BOARD* aBoard )
{
    std::vector<D_PAD*> pads;
    DLIST<MODULE>* modules = &const_cast<BOARD*>( aBoard )->m_Modules;
    m_first_module = modules->GetFirst();
}

void PADS_SCAN_BASE::Execute( void )
{
    MODULE* module = m_first_module;
    while( module )
    {
        D_PAD* pad = module->PadsList();
        while( pad )
        {
            if( ExecutePad( pad ) )
                break;
            pad = pad->Next();
        }
        module = module->Next();
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
// Pick Vias and track sizes.
//---------------------------------------------------------------------------------------------------
void TRACKITEMS::PickViaSize( const VIA* aVia )
{
    VIA_DIMENSION via_dim;
    via_dim.m_Diameter = aVia->GetWidth();
    via_dim.m_Drill = aVia->GetDrillValue();
    unsigned int index = 0;
    for( index = 1; index < m_Board->GetDesignSettings().m_ViasDimensionsList.size(); ++index )
        if( m_Board->GetDesignSettings().m_ViasDimensionsList.at( index ).m_Diameter == via_dim.m_Diameter )
        {
            m_Board->GetDesignSettings().SetViaSizeIndex( index );
            break;
        }
    if( index > m_Board->GetDesignSettings().m_ViasDimensionsList.size() - 1 )
    {
        m_Board->GetDesignSettings().m_ViasDimensionsList.push_back( via_dim );
        std::sort( m_Board->GetDesignSettings().m_ViasDimensionsList.begin() + 1,
                   m_Board->GetDesignSettings().m_ViasDimensionsList.end() );
        for( index = 0; index < m_Board->GetDesignSettings().m_ViasDimensionsList.size(); ++index )
            if( m_Board->GetDesignSettings().m_ViasDimensionsList.at( index ).m_Diameter == via_dim.m_Diameter )
            {
                m_Board->GetDesignSettings().SetViaSizeIndex( index );
                break;
            }
    }
}

void TRACKITEMS::PickTrackSize( const TRACK* aTrack )
{
    int width = aTrack->GetWidth();
    unsigned int index = 0;
    for( index = 1; index < m_Board->GetDesignSettings().m_TrackWidthList.size(); ++index )
        if( m_Board->GetDesignSettings().m_TrackWidthList.at( index ) == width )
        {
            m_Board->GetDesignSettings().SetTrackWidthIndex( index );
            break;
        }
    if( index > m_Board->GetDesignSettings().m_TrackWidthList.size() - 1 )
    {
        m_Board->GetDesignSettings().m_TrackWidthList.push_back( width );
        std::sort( m_Board->GetDesignSettings().m_TrackWidthList.begin() + 1,
                   m_Board->GetDesignSettings().m_TrackWidthList.end() );

        for( index = 0; index < m_Board->GetDesignSettings().m_TrackWidthList.size(); ++index )
            if( m_Board->GetDesignSettings().m_TrackWidthList.at( index ) == width )
            {
                m_Board->GetDesignSettings().SetTrackWidthIndex( index );
                break;
            }
    }
}
//---------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
// DRC
//----------------------------------------------------------------------------------------

MARKER_PCB* TRACKITEMS::DRC_AddMarker( const BOARD_CONNECTED_ITEM* aItem1,
                                       const BOARD_ITEM* aItem2,
                                       const wxPoint aMarkerPos,
                                       const int aErrorCode
                                     )
{
    MARKER_PCB* marker{nullptr};
    if( aItem1 )
    {
        if( aItem2 )
            marker = new MARKER_PCB( aErrorCode,
                                     aMarkerPos,
                                     aItem1->GetSelectMenuText(),
                                     aItem1->GetPosition(),
                                     aItem2->GetSelectMenuText(),
                                     aItem2->GetPosition() );
        else
            marker = new MARKER_PCB( aErrorCode,
                                     aMarkerPos,
                                     aItem1->GetSelectMenuText(),
                                     aItem1->GetPosition() );

        if( marker )
        {
            marker->SetItem( aItem1 );
            m_Board->Add( static_cast<BOARD_ITEM*>( marker ) );
            //GAL canvas
            m_EditFrame->GetGalCanvas()->GetView()->Add( static_cast<BOARD_ITEM*>( marker ) );
        }
    }
    return marker;
}

//----------------------------------------------------------------------------------------
//Show track in 45 and 90 degrees angles when moving segment or node.
//Show node in position.
//----------------------------------------------------------------------------------------
void TRACKITEMS::Angles( const TRACK* aTrackSeg,
                         const wxPoint aPosition,
                         EDA_DRAW_PANEL* aPanel,
                         wxDC* aDC,
                         const wxPoint& aOffset
                       )
{

    EDA_RECT* e_rect = aPanel->GetClipBox();
    wxPoint track_start = aTrackSeg->GetStart();
    wxPoint track_end = aTrackSeg->GetEnd();

    wxPoint op_pos;
    ( track_start == aPosition )? op_pos = track_end : op_pos = track_start;
    double track_angle = TrackSegAngle( aTrackSeg, op_pos );

    int angle_track_mils_int = Rad2MilsInt( track_angle );
#ifdef NEWCONALGO
    COLOR4D color = m_Board->Colors().GetLayerColor( aTrackSeg->GetLayer() );
#else
    COLOR4D color = m_Board->GetLayerColor( aTrackSeg->GetLayer() );
#endif

    int width = aTrackSeg->GetWidth();

    //angle 45 degrees versus grid
    if( ( angle_track_mils_int == RAD_0_MILS_INT ) ||
        ( angle_track_mils_int == RAD_45_MILS_INT ) ||
        ( angle_track_mils_int == RAD_90_MILS_INT ) ||
        ( angle_track_mils_int == RAD_135_MILS_INT ) ||
        ( angle_track_mils_int == RAD_180_MILS_INT ) ||
        ( angle_track_mils_int == RAD_225_MILS_INT ) ||
        ( angle_track_mils_int == RAD_270_MILS_INT ) ||
        ( angle_track_mils_int == RAD_315_MILS_INT ) ||
        ( angle_track_mils_int == RAD_360_MILS_INT ) )
    {
        GRLine( e_rect,
                aDC,
                track_start + aOffset,
                track_end + aOffset,
                width / 20,
                color );
    }

    //angle 45 degrees multiples versus another track in not moving pos.
    Tracks_Container tracks_list;
    TracksConnected( aTrackSeg, op_pos, tracks_list );

    for( TRACK* edittrack_collected : m_edittrack_start_segments )
        tracks_list.insert( edittrack_collected );

    for( TRACK* track_seg : tracks_list )
    {
        if( (track_seg != aTrackSeg) &&
            !track_seg->IsNull() &&
            !dynamic_cast<TRACK*>( const_cast<TRACK*>( aTrackSeg ) )->IsNull() )
        {
            double angle_btw = AngleBtwTracks( aTrackSeg, op_pos, track_seg, op_pos );
            int angle_cmp = Rad2DeciDegRnd( angle_btw );
            //Only angles >= 90 to be shown.
            if( ( angle_cmp == ANGLE_90 ) ||
                ( angle_cmp == ANGLE_270 ) ||
                ( angle_cmp == ANGLE_135 ) ||
                ( angle_cmp == ANGLE_180 ) ||
                ( angle_cmp == ANGLE_225 ) )
            {
                int line_width = width / 40;
                int length = ( aTrackSeg->GetClearance() / 2 ) + ( width / 2 );
                double seg_angle = TrackSegAngle( track_seg, op_pos );

                wxPoint start{0,0}, end{0,0}, square{0,0};
                if( angle_btw > M_PI )
                {
                    start = GetPoint( op_pos, track_angle, length );
                    end = GetPoint( op_pos, seg_angle, length );
                    square = GetPoint( start, seg_angle, length );
                }
                else
                {
                    start = GetPoint( op_pos, seg_angle, length );
                    end = GetPoint( op_pos, track_angle, length );
                    square = GetPoint( end, seg_angle, length );
                }

                wxPoint draw_start = start + aOffset;
                wxPoint draw_end = end + aOffset;
                wxPoint draw_oppos = op_pos + aOffset;
                wxPoint draw_square = square + aOffset;

                if( IsSharpAngle( angle_btw, false ) )
                {
                    GRLine( e_rect, aDC, draw_start, draw_square, line_width, color );
                    GRLine( e_rect, aDC, draw_end, draw_square, line_width, color );
                }
                else
                    GRArc1( e_rect, aDC, draw_start, draw_end, draw_oppos, line_width, color );
            }
        }
    }
}

void TRACKITEMS::Edittrack_Init( const TRACK* aTrackSeg, const wxPoint aPosition )
{
    m_edittrack_start_segments.clear();
    TracksConnected( aTrackSeg, aPosition, m_edittrack_start_segments );
    if( aTrackSeg->Type() == PCB_TRACE_T )
        m_edittrack_start_segments.insert( const_cast<TRACK*>( aTrackSeg ) );
}

void TRACKITEMS::Edittrack_Clear( void )
{
    m_edittrack_start_segments.clear();
}

void TRACKITEMS::Angles( const std::vector<DRAG_SEGM_PICKER>* aDragSegmentList,
                         const wxPoint aPosition,
                         EDA_DRAW_PANEL* aPanel,
                         wxDC* aDC,
                         const wxPoint& aOffset
                       )
{
    EDA_RECT* e_rect = aPanel->GetClipBox();

    std::vector<Angle_Arc_Tuple> angle_arc;
    angle_arc.clear();

    std::size_t drag_seg_list_size = aDragSegmentList->size();
    for( unsigned int n = 0; n < drag_seg_list_size; ++n )
    {
        TRACK* track_seg = aDragSegmentList->at( n ).m_Track;

        //Calc angle 45 degrees multiples versus another track in current pos.
        TRACK* track_cmp = nullptr;
        for( unsigned int m = n + 1; m < drag_seg_list_size; ++m )
        {
            if( n < drag_seg_list_size - 1 )
                track_cmp = aDragSegmentList->at( m ).m_Track;
            else
                track_cmp = aDragSegmentList->at( 0 ).m_Track;

            if( dynamic_cast<TRACK*>( track_seg ) &&
                dynamic_cast<TRACK*>( track_cmp ) &&
                ( track_cmp != track_seg ) &&
                !dynamic_cast<TRACK*>( track_seg )->IsNull() &&
                !dynamic_cast<TRACK*>( track_cmp )->IsNull() )
            {
                double angle_btw = AngleBtwTracks( track_seg, aPosition, track_cmp, aPosition );
                int angle_cmp = Rad2DeciDegRnd( angle_btw );
                //Only angles >= 90 to be shown.
                if( ( angle_cmp == ANGLE_90 ) ||
                    ( angle_cmp == ANGLE_270 ) ||
                    ( angle_cmp == ANGLE_135 ) ||
                    ( angle_cmp == ANGLE_180 ) ||
                    ( angle_cmp == ANGLE_225 ) )
                {
                    int length = ( track_seg->GetClearance() / 2 ) + ( track_seg->GetWidth() / 2 );

                    double seg_angle = TrackSegAngle( track_seg, aPosition );
                    double cmp_angle = TrackSegAngle( track_cmp, aPosition );

                    wxPoint start{0,0}, end{0,0}, square{0,0};
                    if( angle_btw > M_PI )
                    {
                        start = GetPoint( aPosition, seg_angle, length );
                        end = GetPoint( aPosition, cmp_angle, length );
                        square = GetPoint( start, cmp_angle, length );
                    }
                    else
                    {
                        start = GetPoint( aPosition, cmp_angle, length );
                        end = GetPoint( aPosition, seg_angle, length );
                        square = GetPoint( end, cmp_angle, length );
                    }

                    Angle_Arc_Tuple helper_arc;
                    std::get<MID_POS>( helper_arc ) = aPosition;
                    std::get<START_POS>( helper_arc ) = start;
                    std::get<END_POS>( helper_arc ) = end;
                    std::get<RECT_POS>( helper_arc ) = square;
                    std::get<IS_RECT>( helper_arc ) = IsSharpAngle( angle_btw, false );
                    angle_arc.push_back( helper_arc );
                }
            }
        }
    }

    TRACK* track_seg = aDragSegmentList->at( 0 ).m_Track;
#ifdef NEWCONALGO
    COLOR4D color = m_Board->Colors().GetLayerColor( track_seg->GetLayer() );
#else
    COLOR4D color = m_Board->GetLayerColor( track_seg->GetLayer() );
#endif
    int line_width = track_seg->GetWidth() / 40;

    //angle 45 degrees multiples versus another track in current pos.
    //erase
    if( !m_draw_help_angle_arc.empty() )
    {
        for( Angle_Arc_Tuple helper_arc : m_draw_help_angle_arc )
        {
            wxPoint draw_mid = std::get<MID_POS>( helper_arc ) + aOffset;
            wxPoint draw_start = std::get<START_POS>( helper_arc ) + aOffset;
            wxPoint draw_end = std::get<END_POS>( helper_arc ) + aOffset;
            wxPoint draw_square = std::get<RECT_POS>( helper_arc ) + aOffset;

            if( std::get<IS_RECT>( helper_arc ) )
            {
                GRLine( e_rect, aDC, draw_start, draw_square, line_width, color );
                GRLine( e_rect, aDC, draw_end, draw_square, line_width, color );
            }
            else
                GRArc1( e_rect, aDC, draw_start, draw_end, draw_mid, line_width, color );
        }

        m_draw_help_angle_arc.clear();
    }

    //draw
    if( !angle_arc.empty() )
    {
        m_draw_help_angle_arc.clear();
        for( Angle_Arc_Tuple helper_arc : angle_arc )
        {
            wxPoint draw_mid = std::get<MID_POS>( helper_arc ) + aOffset;
            wxPoint draw_start = std::get<START_POS>( helper_arc ) + aOffset;
            wxPoint draw_end = std::get<END_POS>( helper_arc ) + aOffset;
            wxPoint draw_square = std::get<RECT_POS>( helper_arc ) + aOffset;

            if( std::get<IS_RECT>( helper_arc ) )
            {
                GRLine( e_rect, aDC, draw_start, draw_square, line_width, color );
                GRLine( e_rect, aDC, draw_end, draw_square, line_width, color );
            }
            else
                GRArc1( e_rect, aDC, draw_start, draw_end, draw_mid, line_width, color );

            m_draw_help_angle_arc.push_back( helper_arc );
        }
    }
}
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Draw target to node when in pos.
//----------------------------------------------------------------------------------------
TRACKITEMS::NET_SCAN_DRAW_TARGET_NODE_POS::NET_SCAN_DRAW_TARGET_NODE_POS( const TRACK* aStartTrack,
                                                                          const wxPoint aPosition,
                                                                          const std::vector<DRAG_SEGM_PICKER>* aDragSegmentList,
                                                                          const TRACKITEMS* aParent
                                                                        ) :
    NET_SCAN_BASE( aStartTrack, aParent )
{
    m_result = false;
    m_pos = aPosition;
    m_drag_segments = const_cast<std::vector<DRAG_SEGM_PICKER>*>( aDragSegmentList );
}

bool TRACKITEMS::NET_SCAN_DRAW_TARGET_NODE_POS::ExecuteAt( TRACK* aTrack )
{
    if( aTrack != m_scan_start_track )
    {
        if( ( ( aTrack->GetStart() == m_pos ) ||
            ( aTrack->GetEnd() == m_pos ) ) &&
            aTrack->IsOnLayer( m_scan_start_track->GetLayer() ) )
        {
            for( unsigned int n = 0; n < m_drag_segments->size(); ++n )
                if( m_drag_segments->at( n ).m_Track == aTrack )
                    return false;

            m_result = true;
            return true;
        }
    }
    return false;
}

//Is at track pos or at via pos or at pad pos.
void TRACKITEMS::Target( const std::vector<DRAG_SEGM_PICKER>* aDragSegmentList,
                         const wxPoint aPosition,
                         EDA_DRAW_PANEL* aPanel,
                         wxDC* aDC,
                         const wxPoint& aOffset
                       )
{
    TRACK* track_seg = nullptr;
    for( unsigned int n = 0; n < aDragSegmentList->size(); ++n )
    {
        track_seg = aDragSegmentList->at( n ).m_Track;
        if( ( track_seg->GetStart() == aPosition ) || ( track_seg->GetEnd() == aPosition ) )
            break;
    }

    if( track_seg )
    {
        bool at_pad = false;
        BOARD_CONNECTED_ITEM* at_item = m_Board->GetLockPoint( aPosition, track_seg->GetLayerSet() );
        if( at_item && ( at_item->Type() == PCB_PAD_T ) )
            if( at_item->GetPosition() == aPosition )
                at_pad = true;

        NET_SCAN_DRAW_TARGET_NODE_POS  at_track = NET_SCAN_DRAW_TARGET_NODE_POS( track_seg,
                                                                                 aPosition,
                                                                                 aDragSegmentList,
                                                                                 this );
        at_track.Execute();

        EDA_RECT* eRect = aPanel->GetClipBox();
        int radi = track_seg->GetWidth()>>2;
#ifdef NEWCONALGO
        COLOR4D color = m_Board->Colors().GetLayerColor( track_seg->GetLayer() );
#else
        COLOR4D color = m_Board->GetLayerColor( track_seg->GetLayer() );
#endif
        int line_width = radi / 8;

        //erase
        if( m_target_pos_drawn )
        {
            GRCircle( eRect,
                      aDC,
                      m_target_pos.x + aOffset.x,
                      m_target_pos.y + aOffset.y,
                      radi,
                      line_width,
                      color );
            m_target_pos_drawn = false;
        }

        if( at_track.GetResult() || at_pad )
        {
            GRCircle( eRect,
                      aDC,
                      aPosition.x + aOffset.x,
                      aPosition.y + aOffset.y,
                      radi,
                      line_width,
                      color );
            m_target_pos_drawn = true;
        }

        m_target_pos = aPosition;
    }
}

//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Bad via track connection.
//----------------------------------------------------------------------------------------
VIA* TRACKITEMS::GetBadConnectedVia( const TRACK* aTrackSeg,
                                     const wxPoint aTrackPos,
                                     Tracks_Container* aResultList
                                   )
{
    VIA* retvia = nullptr;
    std::unique_ptr<NET_SCAN_VIA_BAD_CONNECTION> examine_via(
        new NET_SCAN_VIA_BAD_CONNECTION( this, aTrackSeg, aTrackPos, aResultList ) );
    if( examine_via )
    {
        examine_via->Execute();
        retvia = examine_via->GetVia();
    }
    return retvia;
}


TRACKITEMS::NET_SCAN_VIA_BAD_CONNECTION::NET_SCAN_VIA_BAD_CONNECTION( const TRACKITEMS* aParent,
                                                                      const TRACK* aStartTrack,
                                                                      const wxPoint aTrackPos,
                                                                      Tracks_Container* aResultList
                                                                    ) :
    NET_SCAN_BASE( aStartTrack, aParent )
{
    m_track_pos = aTrackPos;
    m_result_list =  aResultList;
    m_result_list->clear();
    m_via = nullptr;
}

bool TRACKITEMS::NET_SCAN_VIA_BAD_CONNECTION::ExecuteAt( TRACK* aTrack )
{
    PCB_LAYER_ID layer = m_scan_start_track->GetLayer();

    if( ( aTrack->Type() == PCB_VIA_T ) && ( aTrack->IsOnLayer( layer ) ) )
    {
        wxPoint via_pos = aTrack->GetEnd();
        if( hypot( m_track_pos.x - via_pos.x, m_track_pos.y - via_pos.y ) < aTrack->GetWidth()>>1 )
        {
            m_result_list->insert( const_cast<TRACK*>( aTrack ) );
            m_via = static_cast<VIA*>( const_cast<TRACK*>( aTrack ) );
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Length match.
//----------------------------------------------------------------------------------------

TRACKITEMS::NET_SCAN_NET_LENGTH::NET_SCAN_NET_LENGTH( const TRACKITEMS* aParent,
                                                      const TRACK* aStartTrack
                                                    ) :
    NET_SCAN_BASE( aStartTrack, aParent )
{
    m_netlength = 0.0;
}

bool TRACKITEMS::NET_SCAN_NET_LENGTH::ExecuteAt( TRACK* aTrack )
{
    if( aTrack->Type() == PCB_TRACE_T )
    {
        if( dynamic_cast<ROUNDED_CORNER_TRACK*>( const_cast<TRACK*>( aTrack ) ) )
            m_netlength += dynamic_cast<ROUNDED_CORNER_TRACK*>( const_cast<TRACK*>( aTrack ) )->GetLengthVisible();
        else
            m_netlength += aTrack->GetLength();
    }

    if( aTrack->Type() == PCB_ROUNDEDTRACKSCORNER_T )
        m_netlength += dynamic_cast<TrackNodeItem::ROUNDED_TRACKS_CORNER*>( const_cast<TRACK*>( aTrack ) )->GetLengthVisible();

    return false;
}

double TRACKITEMS::GetNetLength( const TRACK* aTrack )
{
    double length = 0.0;
    if( aTrack && ( aTrack->Type() == PCB_TRACE_T || aTrack->Type() == PCB_ROUNDEDTRACKSCORNER_T ) )
    {
        std::unique_ptr<NET_SCAN_NET_LENGTH> net_length( new NET_SCAN_NET_LENGTH( this, aTrack ) );
        if( net_length )
        {
            net_length->Execute();
            length = net_length->GetLength();
        }
    }
    return length;
}

void TRACKITEMS::SetMsgPanel( const TRACK* aTrack )
{
    if( aTrack && ( aTrack->Type() == PCB_TRACE_T || aTrack->Type() == PCB_ROUNDEDTRACKSCORNER_T ) )
    {
        MSG_PANEL_ITEMS msg_panel_items;
        const_cast<TRACK*>( aTrack )->GetMsgPanelInfo( msg_panel_items );

        if( m_EditFrame )
        {
            wxString length_txt = m_EditFrame->LengthDoubleToString( GetNetLength( aTrack ) );
            if( !msg_panel_items.empty() )
            {
                for( MSG_PANEL_ITEMS::iterator panel_item = msg_panel_items.begin();
                     panel_item != msg_panel_items.end();
                     ++panel_item )
                {
                    if( panel_item->GetUpperText() ==  _( "Length" ) )
                    {
                        panel_item->SetLowerText( length_txt );
                        break;
                    }
                }
                m_EditFrame->SetMsgPanel( msg_panel_items );
            }
            else
                m_EditFrame->AppendMsgPanel( _( "Net Length" ), length_txt, DARKCYAN );
        }
    }
}


//---------------------------------------------------------------------------------------------------
// Speedup m_Track linked list in get best inserton point in class_track.
//---------------------------------------------------------------------------------------------------

//Private function to get first items from speeder list.
TRACK* TRACKITEMS::NETCODE_FIRST_TRACKITEM::GetAndSync( const int aNetCode )
{
    TRACK* item = nullptr;
    if( aNetCode < m_netcode_first_trackitems.size() )
    {
        item = m_netcode_first_trackitems[aNetCode];
        if( item )
        {
            //Sync, but maybe needles work.
            while( item )
            {
                TRACK* back_item = item->Back();
                if( ( back_item && ( back_item->GetNetCode() < aNetCode) ) ||
                    ( !back_item && ( item->GetNetCode() == aNetCode ) ) )
                {
                    m_netcode_first_trackitems[aNetCode] = item;
                    return item;
                }
                item = back_item;
            }
        }
    }
    return item;
}

//Return netcodes first item or first item on netcode less than this.
//Otherwise returns m_Track first item.
TRACK* TRACKITEMS::NETCODE_FIRST_TRACKITEM::GetBestInsertPoint( const int aNetCode )
{
    TRACK* ret_item = nullptr;

    if( aNetCode && m_Board->m_Track )
    {
        //Lists first items netcode is smaller than aNetCode.
        if( aNetCode >= m_Board->m_Track->GetNetCode() )
        {
            //Item at netcode exists.
            if( m_netcode_first_trackitems.size() > aNetCode )
                ret_item = GetAndSync( aNetCode );

            //Item at netcode do not exists.
            if( !ret_item )
            {
                //Check item before next netcodes first item, if exists.
                int netcode = aNetCode + 1;
                while( netcode < m_netcode_first_trackitems.size() )
                {
                    TRACK* item = GetAndSync( netcode );
                    if( item )
                    {
                        item = item->Back();
                        if( item )
                        {
                            ret_item = item;
                            break;
                        }
                    }
                    netcode++;
                }

                //aNetCode is bigger than list last item netcode.
                if( !ret_item )
                    ret_item = m_Board->m_Track.GetLast();
            }
        }
    }

    if( !ret_item )
        ret_item = m_Board->m_Track;

#ifdef DEBUG
    if( ret_item )
    {
        TRACK* back_item = ret_item->Back();
        if( back_item )
            wxASSERT_MSG( back_item->GetNetCode() < aNetCode,
                            "NetCodeFirstTrackItem" );
    }
#endif

    return ret_item;
}

TRACK* TRACKITEMS::NETCODE_FIRST_TRACKITEM::GetFirst( const int aNetCode )
{
    TRACK* ret_item = GetAndSync( aNetCode );
    return ret_item;
}

void TRACKITEMS::NETCODE_FIRST_TRACKITEM::Insert( const TRACK* aTrackItem )
{
    if( aTrackItem )
    {
        int netcode = aTrackItem->GetNetCode();
        if( netcode >= m_netcode_first_trackitems.size() )
            m_netcode_first_trackitems.resize( netcode + 1 );
        TRACK* trackitem = const_cast<TRACK*>( aTrackItem );
        TRACK* trackitem_prev = trackitem->Back();
        if( ( trackitem_prev && ( trackitem_prev->GetNetCode() < netcode ) ) ||
            ( !trackitem_prev && ( trackitem->GetNetCode() == netcode ) ) )
        {
            m_netcode_first_trackitems[netcode] = trackitem;
        }
    }
}

void TRACKITEMS::NETCODE_FIRST_TRACKITEM::Remove( const TRACK* aTrackItem )
{
    if( aTrackItem )
    {
        int netcode = aTrackItem->GetNetCode();
        if( netcode < m_netcode_first_trackitems.size() )
        {
            TRACK* trackitem = const_cast<TRACK*>( aTrackItem );
            if( trackitem == m_netcode_first_trackitems[netcode] )
            {
                trackitem = const_cast<TRACK*>( aTrackItem )->Back();
                if( trackitem && ( trackitem->GetNetCode() == netcode ) )
                    m_netcode_first_trackitems[netcode] = trackitem;
                else
                {
                    trackitem = const_cast<TRACK*>( aTrackItem )->Next();
                    if( trackitem && ( trackitem->GetNetCode() == netcode ) )
                        m_netcode_first_trackitems[netcode] = trackitem;
                    else
                        m_netcode_first_trackitems[netcode] = nullptr;
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
// Sort tracks by netcode
//---------------------------------------------------------------------------------------------------
void TRACKITEMS::SortTracks( void )
{
    if( m_Board->m_Track )
    {
        int num_items = m_Board->m_Track.GetCount();

        std::vector<TRACK*> tracks;
        tracks.reserve( num_items );

        for( int n = 0; n < num_items; ++n )
        {
            m_Board->m_Track->m_Param = n;
            tracks.push_back( m_Board->m_Track.PopFront() );
        }

        auto rule = [] ( const TRACK* ref, const TRACK* cmp )
        {
            if( ref->GetNetCode() == cmp->GetNetCode())
                return ref->m_Param < cmp->m_Param;

            return ref->GetNetCode() < cmp->GetNetCode();
        };

        std::sort( tracks.begin(), tracks.end(), rule );

        for( int n = 0; n < num_items;  ++n )
        {
            m_Board->m_Track.PushBack( tracks[n] );
            m_Board->TrackItems()->NetCodeFirstTrackItem()->Insert( tracks[n] );
        }
    }
}
//---------------------------------------------------------------------------------------------------
